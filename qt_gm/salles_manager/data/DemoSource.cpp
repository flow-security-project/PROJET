#include "DemoSource.h"

#include <algorithm>

#include <QDateTime>
#include <QRandomGenerator>

#include "engine/densite/DensiteEstimator.h"
#include "engine/flux/FluxOrchestrator.h"
#include "engine/securite/IntrusionDetector.h"
#include "models/Alerte.h"

DemoSource::DemoSource(QObject* parent)
    : DataSource(parent)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &DemoSource::onTick);
}

DemoSource::~DemoSource()
{
    qDeleteAll(m_densite);
    qDeleteAll(m_intrusion);
}

void DemoSource::start()
{
    m_tick = 0;
    m_timer.start();
    emit statutSource(true, QStringLiteral("Démo locale"));
    emit logAppend(QStringLiteral("Mode DÉMO démarré — créez une salle pour commencer"));
}

void DemoSource::stop()
{
    m_timer.stop();
}

void DemoSource::creerSalle(const Salle& salle)
{
    if (salle.id.trimmed().isEmpty()) {
        emit erreur(QStringLiteral("L'identifiant de salle est obligatoire."));
        return;
    }
    if (m_salles.contains(salle.id)) {
        emit erreur(QStringLiteral("L'identifiant %1 existe déjà.").arg(salle.id));
        return;
    }

    Salle s = salle;
    s.enLigne = true;
    s.enAttente = false;
    s.occupation = 0;
    s.densite = 0.0;
    s.pushHistorique();

    auto* estimateur = new DensiteEstimator();
    if (s.hauteurPorteMesuree && s.hauteurPorteCm > 0.0)
        estimateur->setHauteurPorteCm(s.hauteurPorteCm);
    else
        estimateur->setHauteurPorteCm(210.0);
    m_densite.insert(s.id, estimateur);

    auto* detecteur = new IntrusionDetector();
    detecteur->setHoraires(s.horaireDebut, s.horaireFin);
    m_intrusion.insert(s.id, detecteur);

    m_salles.insert(s.id, s);
    emit salleAjoutee(s.id);
    emit logAppend(QStringLiteral("SALLE créée — %1 (%2)").arg(s.id, s.nom));
}

void DemoSource::modifierSalle(const Salle& salle)
{
    if (!m_salles.contains(salle.id)) {
        emit erreur(QStringLiteral("Salle inconnue : %1").arg(salle.id));
        return;
    }

    Salle updated = salle;
    const Salle previous = m_salles.value(salle.id);
    updated.occupation = previous.occupation;
    updated.densite = previous.densite;
    updated.nbPersonnesEstime = previous.nbPersonnesEstime;
    updated.tendance = previous.tendance;
    updated.nbEntrees = previous.nbEntrees;
    updated.nbSorties = previous.nbSorties;
    updated.occHist = previous.occHist;
    updated.densHist = previous.densHist;
    updated.entHist = previous.entHist;
    updated.sortHist = previous.sortHist;
    updated.fluxSortieHist = previous.fluxSortieHist;
    updated.fluxSortieAnormal = previous.fluxSortieAnormal;
    updated.derniereAlerteFluxSortieMs = previous.derniereAlerteFluxSortieMs;
    updated.intrusionActive = previous.intrusionActive;
    updated.intrusionDureeS = previous.intrusionDureeS;
    updated.enLigne = true;
    updated.enAttente = false;
    if (m_densite.contains(updated.id) && updated.hauteurPorteMesuree
        && updated.hauteurPorteCm > 0.0) {
        m_densite[updated.id]->setHauteurPorteCm(updated.hauteurPorteCm);
    }
    if (m_intrusion.contains(updated.id))
        m_intrusion[updated.id]->setHoraires(updated.horaireDebut, updated.horaireFin);
    m_salles.insert(updated.id, updated);
    emit salleMiseAJour(updated.id);
    emit logAppend(QStringLiteral("CONFIGURATION modifiée — %1").arg(updated.id));
}

void DemoSource::supprimerSalle(const QString& id)
{
    if (!m_salles.contains(id)) {
        emit erreur(QStringLiteral("Salle inconnue : %1").arg(id));
        return;
    }

    m_salles.remove(id);
    m_fluxAccum.remove(id);
    m_dernierTofMs.remove(id);
    m_presenceToF.remove(id);
    m_dernieresCiblesFlux.remove(id);
    if (auto* estimateur = m_densite.take(id))
        delete estimateur;
    if (auto* detecteur = m_intrusion.take(id))
        delete detecteur;
    emit salleSupprimee(id);
    emit logAppend(QStringLiteral("SALLE supprimée — %1").arg(id));
}

void DemoSource::getHauteurPorte(const QString& salleId)
{
    if (salleId.trimmed().isEmpty()) {
        emit erreur(QStringLiteral("Saisissez un identifiant avant la mesure."));
        return;
    }

    emit logAppend(QStringLiteral("MESURE hauteur de porte — %1").arg(salleId));
    QTimer::singleShot(650, this, [this, salleId]() {
        const double valeur = 205.0
                              + double(QRandomGenerator::global()->bounded(21));
        if (m_salles.contains(salleId)) {
            Salle& s = m_salles[salleId];
            s.hauteurPorteCm = valeur;
            s.hauteurPorteMesuree = true;
            if (m_densite.contains(salleId))
                m_densite[salleId]->setHauteurPorteCm(valeur);
            emit salleMiseAJour(salleId);
        }
        emit hauteurPorteMesuree(salleId, valeur, true,
                                 QStringLiteral("Mesure démo confirmée"));
    });
}

void DemoSource::actualiserSalle(const QString& salleId)
{
    if (!m_salles.contains(salleId))
        return;
    emit salleMiseAJour(salleId);
    emit logAppend(QStringLiteral("ACTUALISATION — %1").arg(salleId));
}

void DemoSource::onTick()
{
    ++m_tick;
    for (auto it = m_salles.begin(); it != m_salles.end(); ++it) {
        Salle& s = it.value();
        const uint hash = qHash(s.id);
        const int scenario = int(hash) % 6;

        double flux = 0.0; // pers/min cible selon le scénario
        switch (scenario) {
        case 0: flux = 1.2; break;              // montée lente
        case 1: flux = 6.0; break;              // montée rapide
        case 2: flux = (m_tick / 60) % 2 == 0 ? 2.0 : -1.0; break; // va-et-vient
        case 3: flux = -1.5; break;             // descente
        case 4:                                 // remplissage puis sortie brusque (F3)
            if (m_tick < 75)
                flux = 8.0;
            else if (m_tick < 87)
                flux = -240.0;
            else
                flux = 0.3;
            break;
        case 5:                                 // présence permanente (F11 intrusion)
            flux = 0.4;
            if (s.occupation < 1)
                s.occupation = 1;
            break;
        }
        if (s.occupation >= s.capacite)
            flux = qMin(flux, -1.0);

        double& accum = m_fluxAccum[s.id];
        accum += flux / 60.0;
        int pas = int(accum);
        if (pas != 0) {
            accum -= double(pas);
            if (pas > 0) {
                s.occupation = qMin(s.capacite, s.occupation + pas);
                s.nbEntrees += pas;
            } else {
                s.occupation = qMax(0, s.occupation + pas);
                s.nbSorties += -pas;
            }
        }

        const double sortieFlux = qMax(0.0, -flux);
        const Salle::DetectionFluxSortie detection
            = s.majDetectionFluxSortie(sortieFlux,
                                       QDateTime::currentMSecsSinceEpoch());
        if (detection.alerte) {
            Alerte a;
            a.salleId = s.id;
            a.salleNom = s.nom;
            a.type = QStringLiteral("flux_sortie");
            a.capteurs = {QStringLiteral("HC-SR04"), QStringLiteral("VL53L0X")};
            a.detail = QStringLiteral(
                "Débit sortant %1 pers/min — seuil μ+3σ : %2 "
                "(μ=%3, σ=%4, fenêtre %5 s)")
                           .arg(detection.debit, 0, 'f', 0)
                           .arg(detection.seuil, 0, 'f', 0)
                           .arg(detection.mu, 0, 'f', 1)
                           .arg(detection.sigma, 0, 'f', 1)
                           .arg(detection.points);
            emit alerte(a);
            emit logAppend(QStringLiteral("ALERTE F3 — %1 : %2")
                               .arg(s.id, a.detail));
        }

        s.tendance = flux;
        verifierIntrusion(s.id, QDateTime::currentMSecsSinceEpoch());
        simulerTof(s, QDateTime::currentMSecsSinceEpoch());
        s.mettreAJourAnticipation();
        s.pushHistorique();
        emit salleMiseAJour(s.id);
    }
    majDecisionsFlux();
}

void DemoSource::majDecisionsFlux()
{
    const QHash<QString, DecisionFlux> decisions
        = FluxOrchestrator::calculer(m_salles, m_groupes, &m_dernieresCiblesFlux);

    for (auto it = m_salles.begin(); it != m_salles.end(); ++it) {
        Salle& salle = it.value();
        const DecisionFlux decision = decisions.value(salle.id);
        const bool change = salle.decisionFlux != decision.decision
                            || salle.redirectionVers != decision.redirectionVers
                            || std::abs(salle.attenteEstimeeMin
                                        - decision.attenteEstimeeMin)
                                   > 0.01;
        salle.decisionFlux = decision.decision;
        salle.redirectionVers = decision.redirectionVers;
        salle.attenteEstimeeMin = decision.attenteEstimeeMin;
        if (!change)
            continue;

        emit salleMiseAJour(salle.id);

        if (decision.decision == QStringLiteral("redirection")) {
            emit logAppend(QStringLiteral("FLUX UNI — %1 saturée : redirection vers %2")
                               .arg(salle.id, decision.redirectionVers));
        } else if (decision.decision == QStringLiteral("attente")) {
            const QString attente = decision.attenteEstimeeMin >= 0.0
                                        ? QStringLiteral("~%1 min")
                                              .arg(int(decision.attenteEstimeeMin + 0.5))
                                        : QStringLiteral("indéterminée");
            emit logAppend(QStringLiteral("FLUX MULTI — %1 saturée : attente %2")
                               .arg(salle.id, attente));
        }
    }
}

void DemoSource::verifierIntrusion(const QString& salleId, qint64 maintenantMs)
{
    if (!m_salles.contains(salleId) || !m_intrusion.contains(salleId))
        return;

    Salle& s = m_salles[salleId];
    const bool presence = s.occupation > 0;
    const IntrusionResultat resultat = m_intrusion[salleId]->verifier(presence, maintenantMs);
    s.intrusionActive = resultat.intrusionActive;
    s.intrusionDureeS = resultat.dureeS;
    if (!resultat.nouvelleAlerte)
        return;

    const QString heureActuelle
        = QDateTime::fromMSecsSinceEpoch(maintenantMs).toString(QStringLiteral("HH:mm"));
    const int minutes = int(resultat.dureeS) / 60;
    const int secondes = int(resultat.dureeS) % 60;

    Alerte a;
    a.salleId = s.id;
    a.salleNom = s.nom;
    a.type = QStringLiteral("intrusion");
    a.capteurs = {QStringLiteral("HC-SR04"), QStringLiteral("VL53L0X")};
    a.detail = QStringLiteral(
        "Présence détectée hors horaires autorisés (%1-%2) — "
        "présence depuis %3 min %4 s, horaire %5")
                   .arg(s.horaireDebut, s.horaireFin)
                   .arg(minutes)
                   .arg(secondes)
                   .arg(heureActuelle);
    a.appelCible = QStringLiteral("Agent surveillance");
    emit alerte(a);
    emit logAppend(QStringLiteral("ALERTE F11 — %1 : %2").arg(s.id, a.detail));
}

void DemoSource::simulerTof(Salle& salle, qint64 maintenantMs)
{
    DensiteEstimator* estimateur = m_densite.value(salle.id);
    if (!estimateur)
        return;

    const double hauteurMm = salle.hauteurPorteMesuree && salle.hauteurPorteCm > 0.0
                                 ? salle.hauteurPorteCm * 10.0
                                 : 2100.0;
    const double presenceProb = std::clamp(double(salle.occupation) / 3.0, 0.0, 1.0);

    // Persistance de présence : passages de ~1-2 s, pas de grésillement
    bool& actif = m_presenceToF[salle.id];
    const double aleaEtat
        = double(QRandomGenerator::global()->bounded(1000)) / 1000.0;
    if (actif) {
        if (aleaEtat > 0.80)
            actif = false; // fin du passage
    } else if (aleaEtat < presenceProb) {
        actif = true;      // nouveau passage
    }

    const double plongeon = 900.0 + double(qMin(salle.occupation, 10)) * 60.0;

    qint64& dernierT = m_dernierTofMs[salle.id];
    if (dernierT == 0)
        dernierT = maintenantMs - 4000;

    for (int i = 0; i < 5; ++i) { // 5 Hz simulé
        dernierT += 200;
        double distanceMm = hauteurMm;
        if (actif) {
            distanceMm = hauteurMm - plongeon
                         - double(QRandomGenerator::global()->bounded(201));
        } else {
            distanceMm = hauteurMm
                         + double(QRandomGenerator::global()->bounded(61)) - 30.0;
        }
        estimateur->ajouterEchantillon(distanceMm, dernierT);
    }

    const DensiteEstimation est = estimateur->estimer(maintenantMs);
    salle.densite = est.surface;
    salle.regime = est.regime;
    salle.confiance = est.confiance;
    salle.nbPersonnesEstime = est.nbPersonnes;
}
