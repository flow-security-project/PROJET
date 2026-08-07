#include "DemoSource.h"

#include <QDateTime>
#include <QRandomGenerator>

DemoSource::DemoSource(QObject* parent)
    : DataSource(parent)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &DemoSource::onTick);
    initSalles();
}

void DemoSource::initSalles()
{
    Salle amphi;
    amphi.id = "AMPHI-A";
    amphi.nom = "Amphithéâtre A";
    amphi.capacite = 300;
    amphi.regime = "surface";
    amphi.confiance = 0.85;
    amphi.horaireDebut = "07:00";
    amphi.horaireFin = "22:00";
    amphi.seuilEvacuation = 95;
    m_salles.insert(amphi.id, amphi);

    Salle b204;
    b204.id = "B204";
    b204.nom = "Salle B204";
    b204.capacite = 30;
    b204.regime = "clustering";
    b204.confiance = 0.97;
    b204.horaireDebut = "07:00";
    b204.horaireFin = "22:00";
    m_salles.insert(b204.id, b204);

    Salle a102;
    a102.id = "A102";
    a102.nom = "Salle A102";
    a102.capacite = 40;
    a102.regime = "clustering";
    a102.confiance = 0.95;
    a102.horaireDebut = "07:00";
    a102.horaireFin = "22:00";
    m_salles.insert(a102.id, a102);

    Salle tp3;
    tp3.id = "TP3";
    tp3.nom = "Salle TP3";
    tp3.capacite = 25;
    tp3.regime = "clustering";
    tp3.confiance = 0.96;
    tp3.horaireDebut = "07:00";
    tp3.horaireFin = "22:00";
    m_salles.insert(tp3.id, tp3);

    Salle couloir;
    couloir.id = "COULOIR-EST";
    couloir.nom = "Couloir Est";
    couloir.capacite = 15;
    couloir.regime = "clustering";
    couloir.confiance = 0.98;
    couloir.horaireDebut = "07:00";
    couloir.horaireFin = "22:00";
    m_salles.insert(couloir.id, couloir);

    // Scénario : A102 hors ligne au démarrage, revient au tick 25
    m_salles["A102"].enLigne = false;
    m_salles["A102"].ledCouleur = "gris";
    m_salles["A102"].lcdLigne1 = "HORS LIGNE";
    m_salles["A102"].lcdLigne2 = "NON FIABLE";

    m_salles["COULOIR-EST"].lcdLigne1 = "Occ: 0/15";
    m_salles["COULOIR-EST"].lcdLigne2 = "En ligne";
    m_salles["B204"].lcdLigne1 = "Occ: 0/30";
    m_salles["B204"].lcdLigne2 = "En ligne";
    m_salles["AMPHI-A"].lcdLigne1 = "Occ: 0/300";
    m_salles["AMPHI-A"].lcdLigne2 = "En ligne";
    m_salles["TP3"].lcdLigne1 = "Occ: 0/25";
    m_salles["TP3"].lcdLigne2 = "En ligne";
}

void DemoSource::start()
{
    m_tsBase = QDateTime::currentMSecsSinceEpoch();
    m_tick = 0;
    m_timer.start();
    emit statutMqtt(true, "Démo simulée");
    emit statutAsterisk("Enregistré (simulé)");
    emit logAppend("Source DÉMO démarrée — 5 salles scénarisées");
    majNoeuds();
    for (const QString& id : m_salles.keys()) {
        Salle& s = m_salles[id];
        s.pushHistorique();
        emit salleMiseAJour(id);
    }
}

void DemoSource::stop()
{
    m_timer.stop();
}

void DemoSource::onTick()
{
    m_tick++;
    majAmphi();
    majB204();
    majTp3();
    majCouloir();
    majA102();

    // Appels en attente -> statut final
    for (auto it = m_appelsPending.begin(); it != m_appelsPending.end();) {
        if (m_tick >= it->finTick) {
            auto a = m_alertes.find(it->ts);
            if (a != m_alertes.end()) {
                a->appelStatut = it->statutFinal;
                a->appelHeure = QDateTime::fromMSecsSinceEpoch(it->ts)
                                    .toString("hh:mm:ss");
                emit alerteModifiee(a.value());
                emit logAppend("APPEL  " + a->salleNom + "  "
                               + a->appelCible + " : " + a->appelStatutTexte());
            }
            it = m_appelsPending.erase(it);
        } else {
            ++it;
        }
    }
}

void DemoSource::majCourbes(const QString& id)
{
    Salle& s = m_salles[id];
    s.majCouleurLed();
    s.pushHistorique();
    emit salleMiseAJour(id);
}

void DemoSource::armerAppel(quint64 ts, const QString& statutFinal, int dansTicks)
{
    AppelPending p;
    p.ts = ts;
    p.statutFinal = statutFinal;
    p.finTick = m_tick + dansTicks;
    m_appelsPending.append(p);
}

/* ---------- AMPHI-A : montée -> saturation 312/300 -> décroissance ---------- */
void DemoSource::majAmphi()
{
    Salle& s = m_salles["AMPHI-A"];
    if (m_tick <= 115) {
        s.occupation = qMin(312, int(8 + m_tick * 2.65));
        s.tendance = 2.65;
    } else if (!m_amphiEvacFinie) {
        s.occupation = qMax(60, 312 - (m_tick - 115) * 2);
        if (s.occupation <= 60)
            m_amphiEvacFinie = true;
    }
    s.densite = s.taux();
    s.regime = "surface";
    s.confiance = 0.85;
    s.anticipationMin = (s.tendance > 0 && s.taux() >= 0.80)
                            ? qMax(0, int((s.capacite - s.occupation) / s.tendance))
                            : -1;

    if (s.taux() >= 0.95 && s.taux() < 0.99 && !m_amphiSaturationAlertee) {
        m_amphiSaturationAlertee = true;
        Alerte a;
        a.ts = m_tsBase + quint64(m_tick) * 1000;
        a.salleId = s.id;
        a.salleNom = s.nom;
        a.type = "saturation";
        a.detail = "312/300 personnes depuis 4 minutes";
        a.score = 0.96;
        a.capteurs = { "ToF" };
        a.appelCible = "Gestionnaire bâtiment";
        a.appelStatut = "en_cours";
        ajouterAlerte(a);
        armerAppel(a.ts, "termine", 4);
    }

    s.lcdLigne1 = QString("Occ: %1/%2").arg(s.occupation).arg(s.capacite);
    s.lcdLigne2 = s.taux() >= 0.95 ? "SATURATION" : "En ligne";
    majCourbes(s.id);
}

/* ---------- B204 : montée -> bousculade -> évacuation 2/3 ---------- */
void DemoSource::majB204()
{
    Salle& s = m_salles["B204"];
    if (!s.evacuationActive && !m_b204EvacFinie) {
        s.occupation = qMin(24, m_tick >= 10 ? int(6 + (m_tick - 10) * 0.9) : 0);
        if (m_tick >= 10) {
            s.tendance = 3.2;
            s.anticipationMin = s.taux() >= 0.80
                                    ? qMax(0, int((s.capacite - s.occupation) / s.tendance))
                                    : 8;
        }
    }
    s.densite = s.taux();
    s.regime = "clustering";
    s.confiance = 0.97;

    // Conditions évacuation (F9)
    if (m_tick >= 75 && s.evacuationActive) {
        s.condAudioPct = 94;
        s.condThermPct = 90;
        s.condSurfacePct = 98;
        s.scoreFusion = 3;
    } else if (m_tick >= 45) {
        s.condAudioPct = 87;
        s.condThermPct = 42;
        s.condSurfacePct = 98;
        s.scoreFusion = (s.condSurfacePct >= s.seuilEvacuation) ? 1 : 0;
    } else {
        s.condAudioPct = 20;
        s.condThermPct = 10;
        s.condSurfacePct = 15;
        s.scoreFusion = 0;
    }

    // Bousculade au tick 45
    if (m_tick == 45 && !m_b204Bousculade) {
        m_b204Bousculade = true;
        Alerte a;
        a.ts = m_tsBase + quint64(m_tick) * 1000;
        a.salleId = s.id;
        a.salleNom = s.nom;
        a.type = "bousculade";
        a.detail = "Score: 0.94 | Audio(94%) + Surface(98%)";
        a.score = 0.94;
        a.capteurs = { "audio", "surface" };
        a.appelCible = "Responsable sécurité";
        a.appelStatut = "en_cours";
        ajouterAlerte(a);
        armerAppel(a.ts, "termine", 4);
    }

    // Évacuation automatique au tick 75
    if (m_tick == 75 && !s.evacuationActive) {
        s.evacuationActive = true;
        s.evacuationDebutTs = m_tsBase + quint64(m_tick) * 1000;
        s.occAvantEvac = 24;
        s.ledCouleur = "rouge";
        s.lcdLigne1 = "EVACUATION ->";
        s.lcdLigne2 = "SORTEZ PAR LA PORTE";
        Alerte a;
        a.ts = m_tsBase + quint64(m_tick) * 1000;
        a.salleId = s.id;
        a.salleNom = s.nom;
        a.type = "evacuation";
        a.detail = "Déclencheurs: Audio(94%) + Surface(98%) — 2/3 critères pendant 3 s";
        a.score = 0.98;
        a.capteurs = { "audio", "surface" };
        a.appelCible = "Responsable sécurité";
        a.appelStatut = "en_cours";
        ajouterAlerte(a);
        armerAppel(a.ts, "termine", 3);
        majEvacuationGlobale();
    }

    // Déroulement de l'évacuation : décroissance + taux
    if (s.evacuationActive) {
        s.dureeEvacS = m_tick - 75;
        const int reste = qMax(0, s.occAvantEvac - (m_tick - 75));
        s.occupation = reste;
        s.tauxEvacuation = s.occAvantEvac > 0
                               ? int(100.0 * (s.occAvantEvac - reste) / s.occAvantEvac)
                               : 100;
        s.lcdLigne1 = "EVACUATION ->";
        s.lcdLigne2 = QString("Restants: %1").arg(reste);
        if (reste == 0) {
            s.evacuationActive = false;
            s.dureeEvacS = 0;
            s.scoreFusion = 0;
            s.condAudioPct = 0;
            s.condThermPct = 0;
            s.condSurfacePct = 0;
            s.lcdLigne1 = "Occ: 0/30";
            s.lcdLigne2 = "En ligne";
            m_b204EvacFinie = true;
            majEvacuationGlobale();
        }
    }

    s.lcdLigne1 = s.lcdLigne1.leftJustified(16);
    s.lcdLigne2 = s.lcdLigne2.leftJustified(16);
    majCourbes(s.id);
}

/* ---------- TP3 : intrusion hors horaires (03h17) ---------- */
void DemoSource::majTp3()
{
    Salle& s = m_salles["TP3"];
    if (m_tick == 60 && !m_tp3Intrusion) {
        m_tp3Intrusion = true;
        s.occupation = 1;
        Alerte a;
        a.ts = m_tsBase + quint64(m_tick) * 1000;
        a.salleId = s.id;
        a.salleNom = s.nom;
        a.type = "intrusion";
        a.detail = "Hors horaires (03h17), présence 2 min 30 s";
        a.score = 0.88;
        a.capteurs = { "A-B" };
        a.appelCible = "Agent de surveillance";
        a.appelStatut = "en_cours";
        ajouterAlerte(a);
        armerAppel(a.ts, "echoue", 5); // l'appel échoue (exemple prototype)
    }
    if (m_tick == 70) {
        s.occupation = 0;
    }
    s.lcdLigne1 = QString("Occ: %1/25").arg(s.occupation);
    s.lcdLigne2 = (m_tick >= 60 && m_tick <= 70) ? "INTRUSION!" : "En ligne";
    majCourbes(s.id);
}

/* ---------- COULOIR-EST : personne immobilisée ---------- */
void DemoSource::majCouloir()
{
    Salle& s = m_salles["COULOIR-EST"];
    s.occupation = 1;
    if (m_tick == 15 && !m_couloirImmobile) {
        m_couloirImmobile = true;
        Alerte a;
        a.ts = m_tsBase + quint64(m_tick) * 1000;
        a.salleId = s.id;
        a.salleNom = s.nom;
        a.type = "immobile";
        a.detail = "Durée: 6 min 12 s | Occupation: 1 | Variance ToF ≈ 0";
        a.score = 0.80;
        a.capteurs = { "ToF", "A-B" };
        a.appelCible = "Infirmerie / Secours";
        a.appelStatut = "en_cours";
        ajouterAlerte(a);
        armerAppel(a.ts, "termine", 3);
    }
    s.lcdLigne1 = "Occ: 1/15";
    s.lcdLigne2 = m_couloirImmobile ? "PERSONNE IMMOBILE" : "En ligne";
    majCourbes(s.id);
}

/* ---------- A102 : revient en ligne puis flux de sortie anormal ---------- */
void DemoSource::majA102()
{
    Salle& s = m_salles["A102"];
    if (m_tick == 25 && !s.enLigne) {
        s.enLigne = true;
        s.lcdLigne1 = "Occ: 0/40";
        s.lcdLigne2 = "En ligne";
        emit logAppend("NŒUD  A102  de nouveau en ligne");
        majNoeuds();
    }
    if (m_tick >= 25 && s.enLigne) {
        s.occupation = qMin(32, 20 + (m_tick - 25) / 3);
    }
    if (m_tick == 120 && !m_a102FluxAlerte) {
        m_a102FluxAlerte = true;
        s.occupation = 5; // chute brutale : μ+3σ
        Alerte a;
        a.ts = m_tsBase + quint64(m_tick) * 1000;
        a.salleId = s.id;
        a.salleNom = s.nom;
        a.type = "flux_sortie";
        a.detail = "Sorties brutales (μ+3σ) : 32 -> 5 personnes en 3 s";
        a.score = 0.91;
        a.capteurs = { "A-B" };
        a.appelCible = "Responsable sécurité";
        a.appelStatut = "en_cours";
        ajouterAlerte(a);
        armerAppel(a.ts, "termine", 3);
    }
    if (m_tick >= 120 && m_a102FluxAlerte)
        s.occupation = qMin(6, 5 + (m_tick - 120) / 4);
    s.lcdLigne1 = QString("Occ: %1/40").arg(s.occupation);
    s.lcdLigne2 = m_a102FluxAlerte ? "FLUX ANORMAL" : "En ligne";
    majCourbes(s.id);
}

/* ---------- Commandes (simulées avec latence) ---------- */
void DemoSource::envoyerConfig(const Salle& cfg)
{
    if (!m_salles.contains(cfg.id))
        return;
    Salle& s = m_salles[cfg.id];
    s.nom = cfg.nom;
    s.capacite = cfg.capacite;
    s.horaireDebut = cfg.horaireDebut;
    s.horaireFin = cfg.horaireFin;
    s.seuilEvacuation = cfg.seuilEvacuation;
    const int lat = 40 + int(QRandomGenerator::global()->bounded(21));
    s.lcdLigne1 = QString("Salle: %1").arg(s.nom);
    s.lcdLigne2 = QString("Cap: %1").arg(s.capacite);
    emit configConfirmee(s.id, QString("salle/%1/config/set | QoS 1").arg(s.id), lat);
    emit logAppend(QString("CONFIG  %1  %2 | cap %3 | %4-%5")
                       .arg(s.id, s.nom)
                       .arg(s.capacite)
                       .arg(s.horaireDebut, s.horaireFin));
    majCourbes(s.id);
}

void DemoSource::commanderTest(const QString& salleId, const QString& composant,
                               const QString& valeur)
{
    if (!m_salles.contains(salleId))
        return;
    Salle& s = m_salles[salleId];
    const int lat = 40 + int(QRandomGenerator::global()->bounded(21));
    bool ok = true;

    if (composant == "led") {
        if (valeur == "stroboscope") {
            s.ledCouleur = "rouge";
            emit logAppend(QString("TEST  %1  LED stroboscope (simulé)").arg(salleId));
        } else {
            s.ledCouleur = valeur;
        }
        emit testRetour(salleId, "led", ok, lat);
    } else if (composant == "lcd") {
        s.lcdLigne1 = "TEST LCD -> OK";
        s.lcdLigne2 = QString("%1").arg(valeur);
        emit testRetour(salleId, "lcd", ok, lat);
    } else {
        emit testRetour(salleId, composant, false, 0);
    }
    majCourbes(salleId);
}

void DemoSource::forcerEvacuation(const QString& salleId, bool actif)
{
    if (!m_salles.contains(salleId))
        return;
    Salle& s = m_salles[salleId];
    s.evacuationActive = actif;
    if (actif) {
        s.occAvantEvac = qMax(1, s.occupation);
        s.evacuationDebutTs = m_tsBase + quint64(m_tick) * 1000;
        s.ledCouleur = "rouge";
        s.lcdLigne1 = "EVACUATION ->";
        s.lcdLigne2 = "FORCEE (MAINTENANCE)";
        Alerte a;
        a.ts = s.evacuationDebutTs;
        a.salleId = s.id;
        a.salleNom = s.nom;
        a.type = "evacuation";
        a.detail = "Déclenchée manuellement (override 3/3)";
        a.score = 1.0;
        a.appelCible = "Responsable sécurité";
        a.appelStatut = "termine";
        a.appelHeure = QDateTime::currentDateTime().toString("hh:mm:ss");
        ajouterAlerte(a);
        emit logAppend("ÉVACUATION FORCÉE  " + s.nom);
    } else {
        s.evacuationActive = false;
        s.lcdLigne1 = QString("Occ: %1/%2").arg(s.occupation).arg(s.capacite);
        s.lcdLigne2 = "En ligne";
        emit logAppend("Évacuation désactivée  " + s.nom);
    }
    majEvacuationGlobale();
    majCourbes(salleId);
}

void DemoSource::resetAlertesSalle(const QString& salleId)
{
    for (auto it = m_alertes.begin(); it != m_alertes.end(); ++it) {
        if (it->salleId == salleId && !it->acquittee) {
            it->acquittee = true;
            emit alerteModifiee(it.value());
        }
    }
    if (m_salles.contains(salleId)) {
        m_salles[salleId].evacuationActive = false;
        m_salles[salleId].lcdLigne2 = "En ligne";
        majEvacuationGlobale();
        majCourbes(salleId);
    }
    emit logAppend("ALERTES acquittées : " + salleId);
}
