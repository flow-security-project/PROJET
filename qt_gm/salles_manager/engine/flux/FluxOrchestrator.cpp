#include "engine/flux/FluxOrchestrator.h"

#include <cmath>

double FluxOrchestrator::rSortieMoyen(const Salle& salle)
{
    const int n = salle.fluxSortieHist.size();
    if (n == 0)
        return 0.0;

    const int debut = qMax(0, n - FenetreRSortieS);
    double somme = 0.0;
    for (int i = debut; i < n; ++i)
        somme += salle.fluxSortieHist.at(i);
    return somme / double(n - debut);
}

double FluxOrchestrator::attenteMin(double rSortiePersMin)
{
    // Attente d'une place : K / R_sortie avec K = 1 personne.
    // Si le flux de sortie est nul, l'attente est indéterminée.
    return rSortiePersMin > 0.1 ? 1.0 / rSortiePersMin : -1.0;
}

QHash<QString, DecisionFlux> FluxOrchestrator::calculer(
    const QHash<QString, Salle>& salles,
    const QHash<QString, Groupe>& groupes,
    QHash<QString, QString>* dernieresCibles,
    double seuilSaturation)
{
    QHash<QString, DecisionFlux> decisions;

    // Rassemblement des membres par groupe.
    QHash<QString, QStringList> membresGroupe;
    for (auto it = salles.cbegin(); it != salles.cend(); ++it) {
        const Salle& salle = it.value();
        if (salle.groupeId.isEmpty())
            continue;
        membresGroupe[salle.groupeId].append(salle.id);
    }

    // Décision par groupe UNI puis par salle (MULTI / indépendante).
    for (auto it = salles.cbegin(); it != salles.cend(); ++it) {
        const Salle& salle = it.value();
        DecisionFlux& decision = decisions[salle.id];
        decision.decision = QStringLiteral("normal");
        decision.redirectionVers.clear();
        decision.attenteEstimeeMin = -1.0;

        if (salle.occupation < 0 || !salle.enLigne || salle.enAttente)
            continue;

        const double taux = salle.taux();
        if (taux < seuilSaturation)
            continue;

        const QString groupeId = salle.groupeId;
        if (!groupeId.isEmpty() && groupes.value(groupeId).mode == ModeFlux::Uni) {
            const Groupe& groupe = groupes.value(groupeId);
            const QStringList& membres = membresGroupe.value(groupeId);

            const QString cibleActuelle =
                dernieresCibles ? dernieresCibles->value(salle.id) : QString();

            // Conserver la cible en cours tant qu'elle reste valable (anti ping-pong),
            // sinon choisir la porte la moins chargée.
            QString cible = cibleActuelle;
            if (cible.isEmpty()
                || !salles.value(cible).enLigne
                || salles.value(cible).occupation < 0
                || salles.value(cible).taux() >= taux - groupe.seuilEcart) {
                cible.clear();
                for (const QString& autreId : membres) {
                    if (autreId == salle.id)
                        continue;
                    const Salle& autre = salles.value(autreId);
                    if (autre.occupation < 0 || !autre.enLigne)
                        continue;
                    const double tauxAutre = autre.taux();
                    if (tauxAutre <= taux - groupe.seuilEcart
                        && (cible.isEmpty() || tauxAutre < salles.value(cible).taux()))
                        cible = autreId;
                }
            }

            if (!cible.isEmpty()) {
                decision.decision = QStringLiteral("redirection");
                decision.redirectionVers = cible;
                if (dernieresCibles)
                    dernieresCibles->insert(salle.id, cible);
                continue;
            }
            if (dernieresCibles)
                dernieresCibles->remove(salle.id);
        }

        // MULTI ou stade MULTI ou UNI sans alternative : attente estimée.
        decision.decision = QStringLiteral("attente");
        decision.attenteEstimeeMin = attenteMin(rSortieMoyen(salle));
    }

    return decisions;
}
