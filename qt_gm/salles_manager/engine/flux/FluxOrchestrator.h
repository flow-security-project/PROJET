#pragma once

#include <QHash>
#include <QList>
#include <QString>

#include "models/Groupe.h"
#include "models/Salle.h"

struct DecisionFlux
{
    QString decision;          // "normal" | "attente" | "redirection"
    QString redirectionVers;   // id de la porte de destination (UNI)
    double attenteEstimeeMin;  // -1 = indéterminée (R_sortie ≈ 0)
};

class FluxOrchestrator
{
public:
    // Fenêtre (secondes) de flux de sortie utilisée pour la moyenne glissante R_sortie.
    static constexpr int FenetreRSortieS = 180;

    // Pour chaque salle présente et en ligne, décide de la conduite à tenir :
    //  - MULTI / indépendante : attente estimée K / R_sortie si saturée ;
    //  - UNI : redirection vers la porte du stade nettement moins chargée,
    //          sinon attente. Anti ping-pong par écart minimal (Groupe::seuilEcart)
    //          et mémorisation de la cible en cours.
    static QHash<QString, DecisionFlux> calculer(
        const QHash<QString, Salle>& salles,
        const QHash<QString, Groupe>& groupes,
        QHash<QString, QString>* dernieresCibles = nullptr,
        double seuilSaturation = 0.95);

private:
    static double rSortieMoyen(const Salle& salle);
    static double attenteMin(double rSortiePersMin);
};
