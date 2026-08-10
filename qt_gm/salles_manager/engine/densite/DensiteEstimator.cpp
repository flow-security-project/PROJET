#include "DensiteEstimator.h"

#include <algorithm>
#include <cmath>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QFile>

namespace {
constexpr int CAPACITE_BUFFER = 60;      // 12 s à 5 Hz
constexpr int FENETRE_SURFACE_MS = 3000; // fenêtre d'estimation surface
constexpr int FENETRE_CLUSTERS_MS = 3000;
constexpr double ZONES_ACTIVES = 3.0;    // zones simultanées maximales de la porte
constexpr int DUREE_MIN_PRESENCE_MS = 200;
constexpr int GAP_MIN_CLUSTERS_MS = 600;
constexpr double MARGE_PERSONNE_MM = 800.0; // personne ≥ 80 cm
constexpr double PORTEE_CLOSENESS_MM = 800.0;
constexpr double SEUIL_PLONGEON_MM = 150.0; // profondeur minimale d'une silhouette
constexpr int HYSTERESIS_MS = 3000;
}

DensiteEstimator::DensiteEstimator()
{
    // Table par défaut empirique (avant calibration in situ F18)
    m_table = {
        {0.0, 0.0},  {0.10, 0.4}, {0.20, 1.0}, {0.35, 2.0},
        {0.50, 3.0}, {0.65, 4.5}, {0.80, 6.5}, {1.00, 10.0},
    };
}

void DensiteEstimator::setHauteurPorteCm(double centimetres)
{
    if (centimetres <= 0.0)
        return;
    m_hauteurPorteMm = centimetres * 10.0;
    m_seuilPresenceMm = m_hauteurPorteMm - MARGE_PERSONNE_MM;
}

void DensiteEstimator::setTableCalibration(const QVector<QPointF>& table)
{
    m_table = table;
    std::sort(m_table.begin(), m_table.end(),
              [](const QPointF& gauche, const QPointF& droite) {
                  return gauche.x() < droite.x();
              });
    m_tableChargee = !m_table.isEmpty();
}

bool DensiteEstimator::chargerCalibration(const QString& cheminJson)
{
    QFile fichier(cheminJson);
    if (!fichier.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError erreur;
    const QJsonDocument doc = QJsonDocument::fromJson(fichier.readAll(), &erreur);
    if (erreur.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    const QJsonObject objet = doc.object();
    const QJsonArray surfaces = objet.value(QStringLiteral("surface")).toArray();
    const QJsonArray personnes = objet.value(QStringLiteral("personnes")).toArray();
    if (surfaces.isEmpty() || surfaces.size() != personnes.size())
        return false;

    QVector<QPointF> table;
    for (int i = 0; i < surfaces.size(); ++i) {
        table.append(QPointF(surfaces.at(i).toDouble(), personnes.at(i).toDouble()));
    }
    setTableCalibration(table);
    return true;
}

void DensiteEstimator::ajouterEchantillon(double distanceMm, qint64 tMs)
{
    if (distanceMm <= 0.0 || tMs <= 0)
        return;
    EchantillonToF e;
    e.distanceMm = distanceMm;
    e.tMs = tMs;
    m_echantillons.append(e);
    while (m_echantillons.size() > CAPACITE_BUFFER)
        m_echantillons.removeFirst();
    m_dernierTMs = tMs;
}

void DensiteEstimator::reset()
{
    m_echantillons.clear();
    m_emaMm = -1.0;
    m_regime = QStringLiteral("clustering");
    m_dernierBasculeMs = 0;
}

void DensiteEstimator::filtrer()
{
    // Médiane 3 puis EMA pour rejeter les trames parasites (target_status != 0)
    if (m_echantillons.size() < 3)
        return;
    const int n = m_echantillons.size();
    QVector<double> fenetre = {m_echantillons.at(n - 3).distanceMm,
                               m_echantillons.at(n - 2).distanceMm,
                               m_echantillons.at(n - 1).distanceMm};
    std::sort(fenetre.begin(), fenetre.end());
    const double valeur = fenetre.at(1);
    m_emaMm = m_emaMm < 0.0 ? valeur : 0.6 * m_emaMm + 0.4 * valeur;
}

double DensiteEstimator::closeness(double distanceMm) const
{
    if (distanceMm >= m_seuilPresenceMm)
        return 0.0;
    return std::clamp((m_seuilPresenceMm - distanceMm) / PORTEE_CLOSENESS_MM,
                      0.0, 1.0);
}

double DensiteEstimator::surfaceOccupee(qint64 maintenantMs) const
{
    if (m_echantillons.isEmpty())
        return 0.0;

    double somme = 0.0;
    int count = 0;
    for (const EchantillonToF& e : m_echantillons) {
        if (maintenantMs - e.tMs > FENETRE_SURFACE_MS)
            continue;
        somme += closeness(e.distanceMm);
        ++count;
    }
    return count > 0 ? somme / double(count) : 0.0;
}

int DensiteEstimator::compterClusters(qint64 maintenantMs) const
{
    if (m_echantillons.size() < 2)
        return 0;

    int clusters = 0;
    int dansPasse = 0;
    qint64 debutPasse = 0;
    double minDansPasse = m_hauteurPorteMm;
    qint64 finDernierCluster = 0;

    const auto validerRun = [&](qint64 fin) {
        const qint64 duree = fin - debutPasse;
        const bool plongeonOk = (m_hauteurPorteMm - minDansPasse) >= SEUIL_PLONGEON_MM;
        // Une silhouette est comptée quelle que soit sa durée (stationnement
        // ou foule continue = 1 cluster), seule une durée minimale filtre le bruit.
        const bool recent = fin >= maintenantMs - FENETRE_CLUSTERS_MS;
        if (duree >= DUREE_MIN_PRESENCE_MS && plongeonOk && recent
            && (finDernierCluster == 0
                || debutPasse - finDernierCluster >= GAP_MIN_CLUSTERS_MS)) {
            ++clusters;
            finDernierCluster = fin;
        }
    };

    for (const EchantillonToF& e : m_echantillons) {
        const double d = e.distanceMm;
        const bool present = d < m_seuilPresenceMm;
        if (present) {
            if (dansPasse == 0) {
                debutPasse = e.tMs;
                minDansPasse = d;
            } else {
                minDansPasse = qMin(minDansPasse, d);
            }
            ++dansPasse;
        } else if (dansPasse > 0) {
            validerRun(e.tMs);
            dansPasse = 0;
        }
    }
    if (dansPasse > 0)
        validerRun(m_echantillons.last().tMs);

    return clusters;
}

double DensiteEstimator::personnesDepuisTable(double surface) const
{
    if (m_table.isEmpty())
        return 0.0;

    if (surface <= m_table.first().x())
        return m_table.first().y();
    if (surface >= m_table.last().x())
        return m_table.last().y();

    for (int i = 1; i < m_table.size(); ++i) {
        if (surface <= m_table.at(i).x()) {
            const QPointF g = m_table.at(i - 1);
            const QPointF d = m_table.at(i);
            const double t = (surface - g.x()) / qMax(1e-9, d.x() - g.x());
            return g.y() + t * (d.y() - g.y());
        }
    }
    return m_table.last().y();
}

void DensiteEstimator::majRegime(const DensiteEstimation& base, qint64 maintenantMs)
{
    if (maintenantMs - m_dernierBasculeMs < HYSTERESIS_MS)
        return;

    if (m_regime == QStringLiteral("clustering")) {
        // Bascule surface : silhouettes non séparables (≥3 personnes)
        // ou clusters < moitié des zones actives alors que l'estimation
        // surface indique une forte occupation (recouvrement significatif).
        const bool haut = base.nbClusters >= 3
                          || (base.nbClusters <= int(ZONES_ACTIVES / 2.0)
                              && base.surface >= 0.45);
        if (haut) {
            m_regime = QStringLiteral("surface");
            m_dernierBasculeMs = maintenantMs;
        }
    } else {
        // Retour clustering : occupation faible, silhouettes de nouveau
        // séparables sur une durée soutenue.
        if (base.surface <= 0.30) {
            m_regime = QStringLiteral("clustering");
            m_dernierBasculeMs = maintenantMs;
        }
    }
}

DensiteEstimation DensiteEstimator::estimer(qint64 maintenantMs)
{
    filtrer();

    DensiteEstimation resultat;
    resultat.surface = surfaceOccupee(maintenantMs);
    resultat.nbClusters = compterClusters(maintenantMs);
    resultat.regime = m_regime;

    const double surface = personnesDepuisTable(resultat.surface);

    majRegime(resultat, maintenantMs);

    if (m_regime == QStringLiteral("surface")) {
        // Le nombre estimé ne descend jamais sous le nombre de silhouettes
        // réellement séparées : borne de sécurité quand la surface est faible
        // (passages alternés) mais les clusters sont nombreux.
        resultat.nbPersonnes = qMax(int(std::round(surface)), resultat.nbClusters);
        resultat.regime = QStringLiteral("surface");
        resultat.confiance = m_tableChargee ? 0.90 : 0.65;
    } else {
        resultat.nbPersonnes = resultat.nbClusters;
        resultat.regime = QStringLiteral("clustering");
        resultat.confiance = resultat.nbClusters >= 1 ? 0.85 : 0.55;
    }

    resultat.bascule = resultat.regime != m_regime;
    m_regime = resultat.regime;
    return resultat;
}
