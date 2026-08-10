#include "MqttSource.h"

#include <algorithm>

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include "engine/densite/DensiteEstimator.h"
#include "engine/securite/IntrusionDetector.h"
#include "models/Alerte.h"

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
            ++salle.nbEntrees;
            if (salle.occupation >= 0)
                salle.occupation = qMin(salle.capacite, salle.occupation + 1);
        } else if (event == QStringLiteral("depart")) {
            ++salle.nbSorties;
            if (salle.occupation >= 0)
                salle.occupation = qMax(0, salle.occupation - 1);
            m_departTimes[id].append(QDateTime::currentMSecsSinceEpoch());
            verifierFluxSortie(id);
        }
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
    } else if (suffix == QStringLiteral("config/confirm")) {
        salle.nom = object.value(QStringLiteral("nom")).toString(salle.nom);
        salle.capacite = object.value(QStringLiteral("capacite")).toInt(salle.capacite);
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
    for (auto it = m_salles.begin(); it != m_salles.end(); ++it) {
        Salle& salle = it.value();
        if (salle.enAttente)
            continue;

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

double MqttSource::mediane(QVector<double> valeurs)
{
    std::sort(valeurs.begin(), valeurs.end());
    const int milieu = valeurs.size() / 2;
    if (valeurs.size() % 2 == 0)
        return (valeurs.at(milieu - 1) + valeurs.at(milieu)) / 2.0;
    return valeurs.at(milieu);
}
