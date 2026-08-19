#include "engine/annonce/SpeakManager.h"

#include <QDateTime>

#include "data/DataSource.h"
#include "models/Alerte.h"
#include "models/Salle.h"

namespace {

constexpr qint64 kCooldownMs = 60LL * 1000LL;     // anti-spam front montant
constexpr qint64 kRepetitionMs = 60LL * 1000LL;   // rappel des états persistants
constexpr qint64 kMinIntervalleMs = 3000LL;       // 1 annonce toutes les 3 s minimum

} // namespace

SpeakManager::SpeakManager(QObject* parent)
    : QObject(parent)
{
    connect(&m_voix, &AnnonceVocale::termine,
            this, &SpeakManager::onVoixTerminee);
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &SpeakManager::onTick);
    m_timer.start();
}

void SpeakManager::setSource(DataSource* source)
{
    if (m_source == source)
        return;
    if (m_source)
        disconnect(m_source, nullptr, this, nullptr);
    m_source = source;
    m_decisionAvant.clear();
    m_evacAvant.clear();
    m_satureAvant.clear();
    if (m_source) {
        connect(m_source, &DataSource::alerte,
                this, &SpeakManager::onAlerte);
    }
}

void SpeakManager::setActif(bool actif)
{
    if (m_actif == actif)
        return;
    m_actif = actif;
    if (!actif) {
        m_voix.arreter();
        m_file.clear();
        m_enCours = Annonce();
    }
}

void SpeakManager::setLangueGlobale(const QString& langue)
{
    if (langue == QStringLiteral("en") || langue == QStringLiteral("fr"))
        m_langueGlobale = langue;
}

void SpeakManager::annoncer(const QString& texte)
{
    if (!m_actif || !m_voix.disponible() || texte.trimmed().isEmpty())
        return;
    insererDansFile({ 7, QString(), texte, m_langueGlobale });
}

void SpeakManager::testVoix()
{
    annoncer(texte(m_langueGlobale,
                   QStringLiteral("Test vocal du système de surveillance."),
                   QStringLiteral("Voice test of the surveillance system.")));
}

void SpeakManager::onTick()
{
    surveillerSalles();
}

void SpeakManager::surveillerSalles()
{
    if (!m_source)
        return;

    const QHash<QString, Salle> salles = m_source->salles();
    const qint64 maintenantMs = QDateTime::currentMSecsSinceEpoch();
    for (auto it = salles.begin(); it != salles.end(); ++it) {
        const Salle& salle = it.value();
        const QString id = salle.id;
        const QString nom = nomSalle(salle);
        const QString langue = langueSalle(salle, m_langueGlobale);

        // --- Évacuation active (priorité 1, répétée toutes les 60 s) ---
        const bool evacAvant = m_evacAvant.value(id, false);
        if (salle.evacuationActive) {
            if (!evacAvant)
                m_dernierRepetitionMs.remove(QStringLiteral("evac:%1").arg(id));
            ajouter(1, QStringLiteral("evac:%1").arg(id),
                    texte(langue,
                          QStringLiteral("Évacuation active dans la salle %1 ! "
                                         "Sortez immédiatement par la porte !"),
                          QStringLiteral("Evacuation active in room %1! Leave "
                                         "immediately through the door!"))
                        .arg(nom),
                    langue,
                    true);
        }
        m_evacAvant.insert(id, salle.evacuationActive);

        // --- Saturation (priorité 3, répétée toutes les 60 s) ---
        const bool sature = salle.occupation >= 0 && salle.taux() >= 0.95;
        const bool satureAvant = m_satureAvant.value(id, false);
        if (sature) {
            if (!satureAvant)
                m_dernierRepetitionMs.remove(QStringLiteral("sat:%1").arg(id));
            ajouter(3, QStringLiteral("sat:%1").arg(id),
                    texte(langue,
                          QStringLiteral("La salle %1 est saturée !"),
                          QStringLiteral("Room %1 is full!"))
                        .arg(nom),
                    langue,
                    true);
        }
        m_satureAvant.insert(id, sature);

        // --- Décision de flux (front montant uniquement) ---
        const QString decisionAvant = m_decisionAvant.value(id);
        if (salle.decisionFlux != decisionAvant) {
            if (salle.decisionFlux == QStringLiteral("redirection")) {
                ajouter(4, QStringLiteral("uni:%1").arg(id),
                        texte(langue,
                              QStringLiteral("La porte %1 est saturée. "
                                             "Redirection vers la porte %2."),
                              QStringLiteral("Door %1 is full. Redirecting "
                                             "to door %2."))
                            .arg(nom, salle.redirectionVers),
                        langue);
            } else if (salle.decisionFlux == QStringLiteral("attente")) {
                const QString attente =
                    salle.attenteEstimeeMin >= 0.0
                        ? texte(langue,
                                QStringLiteral(" Attente estimée %1 minutes."),
                                QStringLiteral(" Estimated wait: %1 minutes."))
                              .arg(int(salle.attenteEstimeeMin + 0.5))
                        : texte(langue,
                                QStringLiteral(" Attente indéterminée."),
                                QStringLiteral(" Estimated wait unknown."));
                ajouter(5, QStringLiteral("multi:%1").arg(id),
                        texte(langue,
                              QStringLiteral("La salle %1 est saturée.%2"),
                              QStringLiteral("Room %1 is full.%2"))
                            .arg(nom, attente),
                        langue);
            } else if (salle.decisionFlux == QStringLiteral("normal")
                       && !decisionAvant.isEmpty()
                       && !salle.evacuationActive && !sature) {
                ajouter(7, QStringLiteral("normal:%1").arg(id),
                        texte(langue,
                              QStringLiteral("La salle %1 est de nouveau "
                                             "accessible."),
                              QStringLiteral("Room %1 is accessible again."))
                            .arg(nom),
                        langue);
            }
            m_decisionAvant.insert(id, salle.decisionFlux);
        }
    }
    Q_UNUSED(maintenantMs)
}

void SpeakManager::onAlerte(const Alerte& alerte)
{
    QString langue = m_langueGlobale;
    if (m_source && m_source->salles().contains(alerte.salleId))
        langue = langueSalle(m_source->salles().value(alerte.salleId),
                             m_langueGlobale);
    const QString nom = alerte.salleNom.isEmpty() ? alerte.salleId : alerte.salleNom;
    if (alerte.type == QStringLiteral("evacuation")) {
        ajouter(1, QStringLiteral("evacA:%1").arg(alerte.salleId),
                texte(langue,
                      QStringLiteral("Évacuation active dans la salle %1 ! "
                                     "Sortez immédiatement par la porte !"),
                      QStringLiteral("Evacuation active in room %1! Leave "
                                     "immediately through the door!"))
                    .arg(nom),
                langue);
    } else if (alerte.type == QStringLiteral("intrusion")) {
        ajouter(2, QStringLiteral("intrusion:%1").arg(alerte.salleId),
                texte(langue,
                      QStringLiteral("Alerte intrusion dans la salle %1. "
                                     "Présence hors horaires autorisés."),
                      QStringLiteral("Intrusion alert in room %1. Presence "
                                     "outside authorized hours."))
                    .arg(nom),
                langue);
    } else if (alerte.type == QStringLiteral("saturation")) {
        ajouter(3, QStringLiteral("satA:%1").arg(alerte.salleId),
                texte(langue,
                      QStringLiteral("La salle %1 est saturée !"),
                      QStringLiteral("Room %1 is full!"))
                    .arg(nom),
                langue);
    } else if (alerte.type == QStringLiteral("bousculade")) {
        ajouter(4, QStringLiteral("bousculade:%1").arg(alerte.salleId),
                texte(langue,
                      QStringLiteral("Risque de bousculade dans la salle %1."),
                      QStringLiteral("Risk of stampede in room %1."))
                    .arg(nom),
                langue);
    } else if (alerte.type == QStringLiteral("flux_sortie")) {
        ajouter(6, QStringLiteral("f3:%1").arg(alerte.salleId),
                texte(langue,
                      QStringLiteral("Flux de sortie anormal à la porte %1."),
                      QStringLiteral("Abnormal exit flow at door %1."))
                    .arg(nom),
                langue);
    } else if (alerte.type == QStringLiteral("immobile")) {
        ajouter(7, QStringLiteral("immobile:%1").arg(alerte.salleId),
                texte(langue,
                      QStringLiteral("Personne immobile détectée dans la "
                                     "salle %1."),
                      QStringLiteral("Motionless person detected in room %1."))
                    .arg(nom),
                langue);
    }
}

void SpeakManager::ajouter(int priorite, const QString& cle,
                           const QString& texte, const QString& langue,
                           bool repetitif)
{
    if (!m_actif || !m_voix.disponible())
        return;

    const qint64 maintenantMs = QDateTime::currentMSecsSinceEpoch();
    if (repetitif) {
        const qint64 dernier = m_dernierRepetitionMs.value(cle, 0);
        if (dernier > maintenantMs - kRepetitionMs)
            return;
        m_dernierRepetitionMs.insert(cle, maintenantMs);
    } else {
        const qint64 dernier = m_dernierParTypeMs.value(cle, 0);
        if (dernier > maintenantMs - kCooldownMs)
            return;
        m_dernierParTypeMs.insert(cle, maintenantMs);
    }

    insererDansFile({ priorite, cle, texte, langue });
}

void SpeakManager::insererDansFile(const Annonce& annonce)
{
    int i = 0;
    while (i < m_file.size() && m_file.at(i).priorite <= annonce.priorite)
        ++i;
    m_file.insert(i, annonce);
    emit annonceFilee(annonce.texte);
    jouerSuivante();
}

void SpeakManager::jouerSuivante()
{
    if (!m_actif || !m_voix.disponible())
        return;

    if (m_enCours.priorite > 0) {
        // Une annonce est en lecture : seule une priorité plus critique
        // l'interrompt ; le reste attend dans la file.
        if (!m_file.isEmpty() && m_file.head().priorite < m_enCours.priorite) {
            m_voix.arreter();
            m_enCours = m_file.dequeue();
            emit annonceEnoncee(m_enCours.texte);
            m_voix.parler(m_enCours.texte, m_enCours.langue);
        }
        return;
    }

    if (m_file.isEmpty())
        return;

    m_enCours = m_file.dequeue();
    emit annonceEnoncee(m_enCours.texte);
    m_voix.parler(m_enCours.texte, m_enCours.langue);
}

void SpeakManager::onVoixTerminee()
{
    m_enCours = Annonce();
    jouerSuivante();
}

QString SpeakManager::nomSalle(const Salle& salle)
{
    return salle.nom.trimmed().isEmpty() ? salle.id : salle.nom;
}

QString SpeakManager::langueSalle(const Salle& salle, const QString& defaut)
{
    return (salle.langue == QStringLiteral("en")
            || salle.langue == QStringLiteral("fr"))
               ? salle.langue
               : defaut;
}

QString SpeakManager::texte(const QString& langue,
                            const QString& fr, const QString& en)
{
    return langue == QStringLiteral("en") ? en : fr;
}