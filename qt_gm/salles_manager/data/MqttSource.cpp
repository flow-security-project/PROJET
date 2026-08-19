#include "MqttSource.h"

#include <algorithm>

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include "engine/densite/DensiteEstimator.h"
#include "engine/flux/FluxOrchestrator.h"
#include "engine/passage/PassageDetectorAB.h"
#include "engine/securite/IntrusionDetector.h"
#include "engine/securite/BousculadeDetector.h"
#include "models/Alerte.h"

namespace {
constexpr double AB_HAUTEUR_DEFAUT_CM = 210.0; // hauteur de porte si non mesurée
constexpr double AB_MARGE_HAUTEUR_CM = 20.0;   // marge sous le linteau pour "bloqué"
}

MqttSource::MqttSource(QObject* parent)
    : DataSource(parent)
{
    connect(&m_client, &MqttClient::messageReceived,
            this, &MqttSource::onMessage);
    connect(&m_client, &MqttClient::stateChanged,
            this, &MqttSource::onMqttState);
    connect(&m_client, &MqttClient::errorMessage,
            this, &MqttSource::onMqttError);
    connect(&m_mesureTimer, &QTimer::timeout,
            this, &MqttSource::onMesureTimeout);
    m_mesureTimer.setSingleShot(true);
    m_watchdogTimer.setInterval(1000);
    connect(&m_watchdogTimer, &QTimer::timeout,
            this, &MqttSource::onWatchdogTimeout);
}

MqttSource::~MqttSource()
{
    qDeleteAll(m_densite);
    qDeleteAll(m_intrusion);
    qDeleteAll(m_bousculade);
    qDeleteAll(m_detecteursPassage);
}

void MqttSource::start()
{
    m_watchdogTimer.start();
    emit statutSource(false, QStringLiteral("Déconnecté"));
    emit logAppend(QStringLiteral("Source MQTT prête — indiquez le broker"));
}

void MqttSource::stop()
{
    m_watchdogTimer.stop();
    m_mesureTimer.stop();
    m_client.disconnectFromHost();
}

void MqttSource::connecter(const QString& host, quint16 port, const QString& clientId)
{
    m_client.setHostname(host);
    m_client.setPort(port);
    m_client.setClientId(clientId);
    m_client.connectToHost();
    emit logAppend(QStringLiteral("MQTT connexion à %1:%2").arg(host).arg(port));
}

void MqttSource::creerSalle(const Salle& salle)
{
    if (salle.id.trimmed().isEmpty()) {
        emit erreur(QStringLiteral("L'identifiant de salle est obligatoire."));
        return;
    }
    if (m_salles.contains(salle.id)) {
        emit erreur(QStringLiteral("L'identifiant %1 existe déjà.").arg(salle.id));
        return;
    }

    Salle pending = salle;
    pending.enAttente = true;
    pending.enLigne = false;
    pending.occupation = -1;
    m_salles.insert(pending.id, pending);
    preparerDensite(pending.id);
    preparerSecurite(pending.id);
    preparerPassageAB(pending.id);
    emit salleAjoutee(pending.id);
    publierConfiguration(pending);
    emit logAppend(QStringLiteral("SALLE %1 créée localement — confirmation MQTT attendue")
                       .arg(pending.id));
}

void MqttSource::modifierSalle(const Salle& salle)
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
    updated.timeHist = previous.timeHist;
    updated.fluxSortieHist = previous.fluxSortieHist;
    updated.fluxSortieAnormal = previous.fluxSortieAnormal;
    updated.derniereAlerteFluxSortieMs = previous.derniereAlerteFluxSortieMs;
    updated.intrusionActive = previous.intrusionActive;
    updated.intrusionDureeS = previous.intrusionDureeS;
    updated.enAttente = true;
    m_salles.insert(updated.id, updated);
    if (m_intrusion.contains(updated.id))
        m_intrusion[updated.id]->setHoraires(updated.horaireDebut, updated.horaireFin);
    emit salleMiseAJour(updated.id);
    publierConfiguration(updated);
}

void MqttSource::supprimerSalle(const QString& id)
{
    if (!m_salles.contains(id)) {
        emit erreur(QStringLiteral("Salle inconnue : %1").arg(id));
        return;
    }

    m_salles.remove(id);
    m_departTimes.remove(id);
    m_dernieresCiblesFlux.remove(id);
    if (auto* detecteur = m_detecteursPassage.take(id))
        delete detecteur;
    if (auto* estimateur = m_densite.take(id)) {
        delete estimateur;
    }
    if (auto* detecteur = m_intrusion.take(id)) {
        delete detecteur;
    }
    emit salleSupprimee(id);
    emit logAppend(QStringLiteral("SALLE supprimée du dashboard — %1 "
                                  "(le nœud reste actif, aucune commande de "
                                  "désactivation n'est prévue par le firmware)")
                       .arg(id));
}

void MqttSource::getHauteurPorte(const QString& salleId)
{
    if (salleId.trimmed().isEmpty()) {
        emit erreur(QStringLiteral("Saisissez un identifiant avant la mesure."));
        return;
    }

    m_mesureId = salleId.trimmed();
    m_mesuresMm.clear();
    m_mesureTimer.start(3500);
    emit logAppend(QStringLiteral("MESURE ToF demandée — %1, attente de mesures valides")
                       .arg(m_mesureId));
}

void MqttSource::actualiserSalle(const QString& salleId)
{
    if (!m_salles.contains(salleId))
        return;

    const QByteArray payload = QByteArrayLiteral("{}");
    const bool sent = m_client.publish(QStringLiteral("salle/%1/etat/get").arg(salleId),
                                       payload);
    if (!sent)
        emit erreur(QStringLiteral("Impossible d'actualiser %1 : MQTT déconnecté.")
                        .arg(salleId));
    else
        emit logAppend(QStringLiteral("ACTUALISATION demandée — %1").arg(salleId));
}

void MqttSource::publierConfiguration(const Salle& salle)
{
    QJsonObject horaires;
    horaires.insert(QStringLiteral("debut"), salle.horaireDebut);
    horaires.insert(QStringLiteral("fin"), salle.horaireFin);

    QJsonObject payload;
    payload.insert(QStringLiteral("nom"), salle.nom);
    payload.insert(QStringLiteral("capacite"), salle.capacite);
    payload.insert(QStringLiteral("seuilEvacuation"), salle.seuilEvacuation);
    payload.insert(QStringLiteral("langue"), salle.langue);
    payload.insert(QStringLiteral("appelNumero"), salle.appelNumero);
    payload.insert(QStringLiteral("hauteurPorte_cm"), salle.hauteurPorteCm);
    payload.insert(QStringLiteral("horaires"), horaires);

    const QString topic = QStringLiteral("salle/%1/config/set").arg(salle.id);
    if (!m_client.publish(topic, QJsonDocument(payload).toJson(QJsonDocument::Compact))) {
        emit logAppend(QStringLiteral("CONFIGURATION en attente — broker déconnecté : %1")
                           .arg(topic));
        return;
    }
    emit logAppend(QStringLiteral("CONFIGURATION envoyée — %1").arg(topic));
}

void MqttSource::onMqttState(MqttClient::State state)
{
    if (state == MqttClient::Connected) {
        m_client.subscribe(QStringLiteral("salle/+/raw/tof"));
        m_client.subscribe(QStringLiteral("salle/+/raw/ultrason"));
        m_client.subscribe(QStringLiteral("salle/+/raw/env"));
        m_client.subscribe(QStringLiteral("salle/+/heartbeat"));
        m_client.subscribe(QStringLiteral("salle/+/etat"));
        m_client.subscribe(QStringLiteral("salle/+/config/confirm"));
        m_client.subscribe(QStringLiteral("salle/+/led/etat"));
        m_client.subscribe(QStringLiteral("salle/+/lcd/etat"));
        emit statutSource(true, QStringLiteral("Connecté"));
        emit logAppend(QStringLiteral("MQTT connecté — abonnements salle/+/... actifs"));
        for (const Salle& salle : m_salles) {
            if (salle.enAttente)
                publierConfiguration(salle);
        }
    } else if (state == MqttClient::Connecting) {
        emit statutSource(false, QStringLiteral("Connexion…"));
    } else {
        emit statutSource(false, QStringLiteral("Déconnecté"));
    }
}

void MqttSource::onMqttError(const QString& message)
{
    emit erreur(QStringLiteral("MQTT : %1").arg(message));
}

void MqttSource::onMessage(const QString& topic, const QByteArray& payload)
{
    const QStringList parts = topic.split('/');
    if (parts.size() < 3 || parts.at(0) != QStringLiteral("salle"))
        return;

    const QString id = parts.at(1);
    const QString suffix = parts.mid(2).join('/');
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    const bool jsonObject = parseError.error == QJsonParseError::NoError
                            && document.isObject();
    const QJsonObject object = jsonObject ? document.object() : QJsonObject();

    if (suffix == QStringLiteral("raw/tof")) {
        const double millimetres = object.value(QStringLiteral("d_mm")).toDouble(-1.0);
        const int status = object.value(QStringLiteral("status")).toInt(4);
        const qint64 tMs = qint64(object.value(QStringLiteral("t_ms")).toDouble(0.0));
        if (id == m_mesureId && status != 4 && millimetres > 0.0) {
            m_mesuresMm.append(millimetres);
            if (m_mesuresMm.size() >= 7)
                finaliserMesure(true, QStringLiteral("Mesure ToF confirmée"));
        }
        if (m_salles.contains(id) && m_densite.contains(id)) {
            m_densite[id]->ajouterEchantillon(millimetres,
                                              tMs > 0 ? tMs
                                                      : QDateTime::currentMSecsSinceEpoch());
        }
        if (m_salles.contains(id)) {
            const Salle& salle = m_salles.value(id);
            const double hauteurCm = salle.hauteurPorteMesuree && salle.hauteurPorteCm > 0.0
                                         ? salle.hauteurPorteCm
                                         : AB_HAUTEUR_DEFAUT_CM;
            const bool bloque = status != 4 && millimetres > 0.0
                                && millimetres / 10.0 < hauteurCm - AB_MARGE_HAUTEUR_CM;
            majEtatToF(id, bloque, tMs > 0 ? tMs : QDateTime::currentMSecsSinceEpoch());
        }
        return;
    }

    if (!m_salles.contains(id))
        return;

    Salle& salle = m_salles[id];
    if (suffix == QStringLiteral("heartbeat")) {
        salle.enLigne = object.value(QStringLiteral("etat")).toString()
                        == QStringLiteral("online");
        salle.dernierHeartbeatMs = QDateTime::currentMSecsSinceEpoch();
        salle.uptimeS = object.value(QStringLiteral("uptime_s")).toInt(salle.uptimeS);
    } else if (suffix == QStringLiteral("raw/ultrason")) {
        const QString event = object.value(QStringLiteral("event")).toString();
        if (event == QStringLiteral("presence")) {
            const qint64 tMs = qint64(object.value(QStringLiteral("t_ms")).toDouble(0.0));
            declencherB(id, tMs > 0 ? tMs : QDateTime::currentMSecsSinceEpoch());
        }
        // "depart" : information seule — la sortie est validée par la séquence B→A.
    } else if (suffix == QStringLiteral("raw/env")) {
        // The current room model keeps the aggregate state; environmental data
        // remains available for the future safety widgets.
    } else if (suffix == QStringLiteral("etat")) {
        salle.occupation = object.value(QStringLiteral("occupation")).toInt(-1);
        salle.capacite = object.value(QStringLiteral("capacite")).toInt(salle.capacite);
        salle.densite = object.value(QStringLiteral("densite")).toDouble(salle.densite);
        if (object.contains(QStringLiteral("en_ligne")))
            salle.enLigne = object.value(QStringLiteral("en_ligne")).toBool(false);
        salle.enAttente = false;
        salle.mettreAJourAnticipation();
        salle.pushHistorique();
        publierAffichage(id);
    } else if (suffix == QStringLiteral("led/etat")) {
        salle.ledCouleurConfirmee = object.value(QStringLiteral("couleur"))
                                        .toString(salle.ledCouleurConfirmee);
        salle.ledMode = object.value(QStringLiteral("mode"))
                            .toString(salle.ledMode);
    } else if (suffix == QStringLiteral("lcd/etat")) {
        salle.lcdLigne1 = object.value(QStringLiteral("ligne1"))
                              .toString(salle.lcdLigne1);
        salle.lcdLigne2 = object.value(QStringLiteral("ligne2"))
                              .toString(salle.lcdLigne2);
    } else if (suffix == QStringLiteral("config/confirm")) {
        salle.nom = object.value(QStringLiteral("nom")).toString(salle.nom);
        salle.capacite = object.value(QStringLiteral("capacite")).toInt(salle.capacite);
        if (object.contains(QStringLiteral("langue"))) {
            const QString langue = object.value(QStringLiteral("langue")).toString();
            if (langue == QStringLiteral("fr") || langue == QStringLiteral("en"))
                salle.langue = langue;
        }
        if (object.contains(QStringLiteral("appelNumero"))) {
            const QString num = object.value(QStringLiteral("appelNumero")).toString();
            if (!num.isEmpty())
                salle.appelNumero = num;
        }
        const QJsonObject horaires = object.value(QStringLiteral("horaires")).toObject();
        salle.horaireDebut = horaires.value(QStringLiteral("debut"))
                                 .toString(salle.horaireDebut);
        salle.horaireFin = horaires.value(QStringLiteral("fin"))
                               .toString(salle.horaireFin);
        if (m_intrusion.contains(id))
            m_intrusion[id]->setHoraires(salle.horaireDebut, salle.horaireFin);
        if (object.contains(QStringLiteral("hauteurPorte_cm"))) {
            salle.hauteurPorteCm = object.value(QStringLiteral("hauteurPorte_cm"))
                                       .toDouble(salle.hauteurPorteCm);
            salle.hauteurPorteMesuree = salle.hauteurPorteCm > 0.0;
            if (m_densite.contains(id))
                m_densite[id]->setHauteurPorteCm(salle.hauteurPorteCm);
        }
        salle.enAttente = false;
        publierAffichage(id);
    } else {
        return;
    }

    emit salleMiseAJour(id);
}

void MqttSource::onMesureTimeout()
{
    if (m_mesureId.isEmpty())
        return;
    finaliserMesure(false, QStringLiteral("Aucune mesure ToF valide reçue"));
}

void MqttSource::onWatchdogTimeout()
{
    const qint64 maintenant = QDateTime::currentMSecsSinceEpoch();
    verifierExpirationAB(maintenant);
    for (auto it = m_salles.begin(); it != m_salles.end(); ++it) {
        Salle& salle = it.value();
        if (salle.enAttente)
            continue;

        publierAffichage(salle.id);

        if (salle.enLigne && salle.dernierHeartbeatMs > 0
            && maintenant - salle.dernierHeartbeatMs > 30000) {
            salle.enLigne = false;
            emit salleMiseAJour(salle.id);
            emit logAppend(QStringLiteral("HORS LIGNE — %1 : aucun heartbeat depuis 30 s")
                               .arg(salle.id));
            continue;
        }

        if (salle.enLigne && salle.occupation >= 0) {
            salle.mettreAJourAnticipation();
            salle.pushHistorique();
            verifierFluxSortie(salle.id);
            verifierIntrusion(salle.id, maintenant);
            verifierBousculade(salle.id, maintenant);
            if (m_densite.contains(salle.id)) {
                const DensiteEstimation est
                    = m_densite[salle.id]->estimer(maintenant);
                salle.densite = est.surface;
                salle.regime = est.regime;
                salle.confiance = est.confiance;
                salle.nbPersonnesEstime = est.nbPersonnes;
            }
            emit salleMiseAJour(salle.id);
        }
    }
    majDecisionsFlux();
}

void MqttSource::publierDecisionFlux(const Salle& salle)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("mode"), salle.decisionFlux);
    if (salle.decisionFlux == QStringLiteral("redirection")
        && !salle.redirectionVers.isEmpty()) {
        payload.insert(QStringLiteral("vers"), salle.redirectionVers);
    } else if (salle.decisionFlux == QStringLiteral("attente")
               && salle.attenteEstimeeMin >= 0.0) {
        payload.insert(QStringLiteral("attente_min"),
                       qMax(1.0, std::round(salle.attenteEstimeeMin)));
    }

    const QString topic = QStringLiteral("salle/%1/flux/decision").arg(salle.id);
    if (!m_client.publish(topic, QJsonDocument(payload).toJson(QJsonDocument::Compact))) {
        emit logAppend(QStringLiteral("DÉCISION FLUX en attente — broker déconnecté : %1")
                           .arg(topic));
        return;
    }
    emit logAppend(QStringLiteral("DÉCISION FLUX envoyée — %1").arg(topic));
}

void MqttSource::publierAffichage(const QString& salleId)
{
    if (!m_salles.contains(salleId))
        return;

    Salle& salle = m_salles[salleId];
    const Salle::ChangementAffichage changement = salle.majAffichageLedLcd();
    if (!changement.ledChanged && !changement.lcdChanged)
        return;

    if (changement.ledChanged) {
        QJsonObject ledPayload;
        ledPayload.insert(QStringLiteral("couleur"), salle.ledCouleur);
        ledPayload.insert(QStringLiteral("luminosite"), 80);
        const QString topic = QStringLiteral("salle/%1/led/set").arg(salleId);
        if (!m_client.publish(topic, QJsonDocument(ledPayload).toJson(QJsonDocument::Compact)))
            emit logAppend(QStringLiteral("LED en attente — broker déconnecté : %1").arg(topic));
        else
            emit logAppend(QStringLiteral("LED publiée — %1 : %2").arg(topic, salle.ledCouleur));
    }

    if (changement.lcdChanged) {
        QJsonObject lcdPayload;
        lcdPayload.insert(QStringLiteral("ligne1"), salle.lcdLigne1);
        lcdPayload.insert(QStringLiteral("ligne2"), salle.lcdLigne2);
        const QString topic = QStringLiteral("salle/%1/lcd/set").arg(salleId);
        if (!m_client.publish(topic, QJsonDocument(lcdPayload).toJson(QJsonDocument::Compact)))
            emit logAppend(QStringLiteral("LCD en attente — broker déconnecté : %1").arg(topic));
        else
            emit logAppend(QStringLiteral("LCD publié — %1 : [%2]/[%3]")
                               .arg(topic, salle.lcdLigne1, salle.lcdLigne2));
    }

    emit salleMiseAJour(salleId);
}

void MqttSource::majDecisionsFlux()
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

        publierDecisionFlux(salle);
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

void MqttSource::finaliserMesure(bool succes, const QString& note)
{
    const QString id = m_mesureId;
    const QVector<double> mesures = m_mesuresMm;
    m_mesureId.clear();
    m_mesuresMm.clear();
    m_mesureTimer.stop();

    if (!succes || mesures.isEmpty()) {
        emit hauteurPorteMesuree(id, -1.0, false, note);
        emit erreur(QStringLiteral("Mesure de hauteur échouée pour %1.").arg(id));
        return;
    }

    const double centimetres = mediane(mesures) / 10.0;
    if (m_salles.contains(id)) {
        Salle& salle = m_salles[id];
        salle.hauteurPorteCm = centimetres;
        salle.hauteurPorteMesuree = true;
        if (m_densite.contains(id))
            m_densite[id]->setHauteurPorteCm(centimetres);
        emit salleMiseAJour(id);
    }
    emit hauteurPorteMesuree(id, centimetres, true,
                             QStringLiteral("%1 cm, médiane de %2 mesures")
                                 .arg(centimetres, 0, 'f', 1)
                                 .arg(mesures.size()));
}

void MqttSource::preparerDensite(const QString& salleId)
{
    if (m_densite.contains(salleId))
        return;

    auto* estimateur = new DensiteEstimator();
    const Salle& salle = m_salles.value(salleId);
    if (salle.hauteurPorteMesuree && salle.hauteurPorteCm > 0.0)
        estimateur->setHauteurPorteCm(salle.hauteurPorteCm);

    // Calibration in situ optionnelle (F18) : calibration_{id}.json
    const QString chemin
        = QDir::current().filePath(QStringLiteral("calibration_%1.json").arg(salleId));
    if (estimateur->chargerCalibration(chemin))
        emit logAppend(QStringLiteral("F17 — calibration chargée pour %1 (%2)")
                           .arg(salleId, chemin));
    else
        emit logAppend(QStringLiteral("F17 — %1 : table par défaut (calibration %2 absente)")
                           .arg(salleId, chemin));

    m_densite.insert(salleId, estimateur);
}

void MqttSource::verifierFluxSortie(const QString& salleId)
{
    if (!m_salles.contains(salleId))
        return;

    const qint64 maintenant = QDateTime::currentMSecsSinceEpoch();
    QVector<qint64>& departs = m_departTimes[salleId];
    while (!departs.isEmpty() && maintenant - departs.first() > 60000)
        departs.removeFirst();

    const double debit = double(departs.size()); // départs/min glissants
    Salle& salle = m_salles[salleId];
    const Salle::DetectionFluxSortie detection
        = salle.majDetectionFluxSortie(debit, maintenant);
    if (!detection.alerte)
        return;

    Alerte a;
    a.salleId = salle.id;
    a.salleNom = salle.nom;
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
    emit logAppend(QStringLiteral("ALERTE F3 — %1 : %2").arg(salle.id, a.detail));
}

void MqttSource::preparerSecurite(const QString& salleId)
{
    if (m_intrusion.contains(salleId))
        return;

    auto* detecteur = new IntrusionDetector();
    const Salle& salle = m_salles.value(salleId);
    detecteur->setHoraires(salle.horaireDebut, salle.horaireFin);
    m_intrusion.insert(salleId, detecteur);

    if (!m_bousculade.contains(salleId)) {
        auto* bousculade = new BousculadeDetector();
        m_bousculade.insert(salleId, bousculade);
    }
}

void MqttSource::verifierIntrusion(const QString& salleId, qint64 maintenantMs)
{
    if (!m_salles.contains(salleId) || !m_intrusion.contains(salleId))
        return;

    Salle& salle = m_salles[salleId];
    const bool presence = salle.occupation > 0;
    const IntrusionResultat resultat = m_intrusion[salleId]->verifier(presence, maintenantMs);
    salle.intrusionActive = resultat.intrusionActive;
    salle.intrusionDureeS = resultat.dureeS;
    if (!resultat.nouvelleAlerte)
        return;

    const QString heureActuelle
        = QDateTime::fromMSecsSinceEpoch(maintenantMs).toString(QStringLiteral("HH:mm"));
    const int minutes = int(resultat.dureeS) / 60;
    const int secondes = int(resultat.dureeS) % 60;

    Alerte a;
    a.salleId = salle.id;
    a.salleNom = salle.nom;
    a.type = QStringLiteral("intrusion");
    a.capteurs = {QStringLiteral("HC-SR04"), QStringLiteral("VL53L0X")};
    a.detail = QStringLiteral(
        "Présence détectée hors horaires autorisés (%1-%2) — "
        "présence depuis %3 min %4 s, horaire %5")
                   .arg(salle.horaireDebut, salle.horaireFin)
                   .arg(minutes)
                   .arg(secondes)
                   .arg(heureActuelle);
    a.appelCible = QStringLiteral("Agent surveillance");
    emit alerte(a);
    emit logAppend(QStringLiteral("ALERTE F11 — %1 : %2").arg(salle.id, a.detail));
}

void MqttSource::verifierBousculade(const QString& salleId, qint64 maintenantMs)
{
    if (!m_salles.contains(salleId) || !m_bousculade.contains(salleId))
        return;

    Salle& salle = m_salles[salleId];
    const BousculadeDetector::Resultat res = m_bousculade[salleId]->verifier(salle, maintenantMs);
    if (!res.alerte)
        return;

    Alerte a;
    a.salleId = salle.id;
    a.salleNom = salle.nom;
    a.type = QStringLiteral("bousculade");
    a.capteurs = {QStringLiteral("HC-SR04"), QStringLiteral("VL53L0X")};
    a.detail = QStringLiteral(
        "Risque de bousculade détecté — saturation %1% + flux sortant élevé depuis %2 s")
                .arg(QString::number(salle.taux() * 100.0, 'f', 0))
                .arg(int(res.dureeS));
    a.appelCible = QStringLiteral("Agent surveillance");
    emit alerte(a);
    emit logAppend(QStringLiteral("ALERTE BOUSCULADE — %1 : %2")
                       .arg(salle.id, a.detail));
}

void MqttSource::preparerPassageAB(const QString& salleId)
{
    if (m_detecteursPassage.contains(salleId))
        return;

    auto* detecteur = new PassageDetectorAB();
    connect(detecteur, &PassageDetectorAB::passageValide, this,
            [this, salleId](const QString& direction) {
                emit etatAB(salleId, QStringLiteral("attente"),
                            QDateTime::currentMSecsSinceEpoch());
                validerPassage(salleId, direction);
            });
    connect(detecteur, &PassageDetectorAB::capteurAActive, this,
            [this, salleId] {
                emit etatAB(salleId, QStringLiteral("vu_a"),
                            QDateTime::currentMSecsSinceEpoch());
                emit logAppend(QStringLiteral("PASSAGE A-B — %1 : capteur A activé (ToF)")
                                   .arg(salleId));
            });
    connect(detecteur, &PassageDetectorAB::capteurBActive, this,
            [this, salleId] {
                emit etatAB(salleId, QStringLiteral("vu_b"),
                            QDateTime::currentMSecsSinceEpoch());
                emit logAppend(QStringLiteral("PASSAGE A-B — %1 : capteur B activé "
                                              "(ultrason)")
                                   .arg(salleId));
            });
    connect(detecteur, &PassageDetectorAB::sequenceAnnulee, this,
            [this, salleId] {
                emit etatAB(salleId, QStringLiteral("attente"),
                            QDateTime::currentMSecsSinceEpoch());
                emit logAppend(QStringLiteral("PASSAGE A-B — %1 : séquence annulée "
                                              "(délai dépassé)")
                                   .arg(salleId));
            });
    m_detecteursPassage.insert(salleId, detecteur);
}

void MqttSource::majEtatToF(const QString& salleId, bool bloque, qint64 tMs)
{
    if (PassageDetectorAB* detecteur = m_detecteursPassage.value(salleId))
        detecteur->majToF(bloque, tMs);
}

void MqttSource::declencherB(const QString& salleId, qint64 tMs)
{
    if (PassageDetectorAB* detecteur = m_detecteursPassage.value(salleId))
        detecteur->declencherUltrason(tMs);
}

void MqttSource::validerPassage(const QString& salleId, const QString& direction)
{
    if (!m_salles.contains(salleId))
        return;

    Salle& salle = m_salles[salleId];
    const qint64 maintenant = QDateTime::currentMSecsSinceEpoch();
    if (direction == QStringLiteral("entree")) {
        ++salle.nbEntrees;
        if (salle.occupation >= 0)
            salle.occupation = qMin(salle.capacite, salle.occupation + 1);
        emit logAppend(QStringLiteral("PASSAGE A-B — %1 : ENTRÉE confirmée (A→B)")
                           .arg(salleId));
    } else {
        ++salle.nbSorties;
        if (salle.occupation >= 0)
            salle.occupation = qMax(0, salle.occupation - 1);
        m_departTimes[salleId].append(maintenant);
        verifierFluxSortie(salleId);
        emit logAppend(QStringLiteral("PASSAGE A-B — %1 : SORTIE confirmée (B→A)")
                           .arg(salleId));
    }
    emit passageValide(salleId, direction, maintenant);
    emit salleMiseAJour(salleId);
}

void MqttSource::verifierExpirationAB(qint64 maintenantMs)
{
    for (auto it = m_detecteursPassage.cbegin(); it != m_detecteursPassage.cend(); ++it)
        it.value()->verifierExpiration(maintenantMs);
}

double MqttSource::mediane(QVector<double> valeurs)
{
    std::sort(valeurs.begin(), valeurs.end());
    const int milieu = valeurs.size() / 2;
    if (valeurs.size() % 2 == 0)
        return (valeurs.at(milieu - 1) + valeurs.at(milieu)) / 2.0;
    return valeurs.at(milieu);
}
