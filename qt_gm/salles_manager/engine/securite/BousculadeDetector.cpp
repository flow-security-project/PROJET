#include "BousculadeDetector.h"

#include <QDateTime>
#include <QDebug>

#include "models/Salle.h"

BousculadeDetector::BousculadeDetector(QObject* parent)
    : QObject(parent)
{
}

void BousculadeDetector::setSeuils(double tauxSaturation, double debitSortieSeuil, int dureeMinS)
{
    m_tauxSaturation = tauxSaturation;
    m_debitSortieSeuil = debitSortieSeuil;
    m_dureeMinS = dureeMinS;
}

BousculadeDetector::Resultat BousculadeDetector::verifier(const Salle& salle, qint64 maintenantMs)
{
    Resultat res;
    const QString id = salle.id;

    // Conditions de déclenchement :
    // 1. Salle saturée (taux >= 95%)
    // 2. FLUX DE SORTIE ANORMAL (μ+3σ) OU débit sortant élevé (> seuil)
    // Les deux conditions doivent être vraies simultanément pendant au moins m_dureeMinS

    const bool sature = salle.occupation >= 0 && salle.taux() >= m_tauxSaturation;
    const bool fluxSortieAnormal = salle.fluxSortieAnormal;
    const double debitSortie = (salle.occupation >= 0 && !salle.fluxSortieHist.isEmpty())
                                   ? salle.fluxSortieHist.last() // dernier débit sortant enregistré
                                   : 0.0;
    const bool debitEleve = debitSortie >= m_debitSortieSeuil;

    const bool condition = sature && (fluxSortieAnormal || debitEleve);

    EtatSalle& etat = m_etats[id];

    if (condition) {
        if (!etat.active) {
            // Première détection : démarrer le chrono
            etat.active = true;
            etat.debutMs = maintenantMs;
        } else {
            // Déjà actif : vérifier durée
            const double dureeS = double(maintenantMs - etat.debutMs) / 1000.0;
            res.dureeS = dureeS;
            res.bousculadeActive = true;
            if (dureeS >= m_dureeMinS) {
                res.alerte = true;
                emit bousculadeDetectee(salle.id, dureeS);
            }
        }
    } else {
        // Conditions non remplies : reset si était actif
        if (etat.active) {
            etat.active = false;
            etat.debutMs = 0;
        }
        res.bousculadeActive = false;
        res.dureeS = 0.0;
    }

    return res;
}

void BousculadeDetector::reset()
{
    m_etats.clear();
}