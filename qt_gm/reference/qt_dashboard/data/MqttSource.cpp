#include "MqttSource.h"

#include <QJsonDocument>
#include <QJsonObject>

MqttSource::MqttSource(QObject* parent)
    : DataSource(parent)
{
    connect(&m_client, &MqttClient::stateChanged, this, &MqttSource::onEtatMqtt);
    connect(&m_client, &MqttClient::messageReceived, this, &MqttSource::onMessage);
}

void MqttSource::connecter(const QString& host, quint16 port, const QString& clientId)
{
    m_clientId = clientId;
    m_client.setHostname(host);
    m_client.setPort(port);
    m_client.setClientId(clientId);
    m_client.connectToHost();
    emit logAppend(QString("MQTT  connexion à %1:%2 (%3)").arg(host).arg(port).arg(clientId));
}

void MqttSource::start()
{
    emit statutAsterisk("Enregistré");
    emit logAppend("Source MQTT active — en attente du broker");
}

void MqttSource::stop()
{
    m_client.disconnectFromHost();
}

void MqttSource::onEtatMqtt(MqttClient::State state)
{
    switch (state) {
    case MqttClient::Connected: {
        m_client.subscribe("salle/+/heartbeat", 0);
        m_client.subscribe("salle/+/raw/tof", 0);
        m_client.subscribe("salle/+/raw/ultrason", 0);
        m_client.subscribe("salle/+/raw/env", 0);
        m_client.subscribe("salle/+/audio", 0);
        m_client.subscribe("salle/+/led/etat", 0);
        m_client.subscribe("salle/+/lcd/etat", 0);
        m_client.subscribe("salle/+/config/confirm", 0);
        m_client.subscribe("salle/+/test/result", 0);
        emit statutMqtt(true, "Connecté");
        emit logAppend("MQTT  connecté + abonnements salle/+/...");
        break;
    }
    case MqttClient::Connecting:
        emit statutMqtt(false, "Connexion…");
        break;
    case MqttClient::Disconnected:
    default:
        emit statutMqtt(false, "Déconnecté");
        break;
    }
}

void MqttSource::onMessage(const QString& topic, const QByteArray& payload)
{
    const QStringList parts = topic.split('/');
    if (parts.size() < 3)
        return;
    const QString id = parts.at(1);
    const QString suffix = parts.mid(2).join('/');

    if (!m_salles.contains(id)) {
        Salle s;
        s.id = id;
        s.nom = id;
        s.occupation = -1; // non calculé sans moteur A-B
        m_salles.insert(id, s);
        emit logAppend("NŒUD  " + id + "  découvert (" + topic + ")");
    }
    Salle& s = m_salles[id];

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    const bool jsonOk = (err.error == QJsonParseError::NoError && doc.isObject());

    if (suffix == "heartbeat") {
        const QString etat = jsonOk ? doc.object().value("etat").toString() : QString();
        s.enLigne = (etat == "online");
        s.ledCouleur = s.enLigne ? "vert" : "gris";
        emit logAppend("HEARTBEAT  " + id + "  " + (s.enLigne ? "online" : "offline"));
        majNoeuds();
    } else if (jsonOk) {
        const QJsonObject obj = doc.object();
        if (suffix == "raw/tof") {
            s.tofMm = obj.value("d_mm").toDouble(-1);
        } else if (suffix == "raw/ultrason") {
            s.ultraCm = obj.value("d_cm").toDouble(-1);
        } else if (suffix == "raw/env") {
            s.temp = obj.value("t_c").toDouble(-99);
            s.hr = obj.value("hr").toDouble(-1);
        } else if (suffix == "audio") {
            s.rms = obj.value("rms").toDouble(-1);
        } else if (suffix == "led/etat") {
            s.ledCouleur = obj.value("couleur").toString();
        } else if (suffix == "lcd/etat") {
            s.lcdLigne1 = obj.value("ligne1").toString();
            s.lcdLigne2 = obj.value("ligne2").toString();
        } else if (suffix == "config/confirm") {
            s.nom = obj.value("nom").toString();
            s.capacite = obj.value("capacite").toInt(30);
            const QJsonObject hor = obj.value("horaires").toObject();
            s.horaireDebut = hor.value("debut").toString();
            s.horaireFin = hor.value("fin").toString();
            emit configConfirmee(id, "config/confirm reçue", 0);
        } else if (suffix == "test/result") {
            const bool ok = obj.value("resultat").toString() == "OK";
            emit testRetour(id, obj.value("composant").toString(), ok,
                            obj.value("latence_ms").toInt());
        }
    }

    // Throttle : mise à jour IHM max 1 Hz par salle (tof = 5 Hz en brut)
    const quint64 now = quint64(QDateTime::currentMSecsSinceEpoch());
    majSalle(id, now);
}

void MqttSource::majSalle(const QString& id, quint64 maintenantMs)
{
    const quint64 dernier = m_derniereMaj.value(id, 0);
    if (maintenantMs - dernier < 1000)
        return;
    m_derniereMaj[id] = maintenantMs;
    if (!m_salles.contains(id))
        return;
    Salle& s = m_salles[id];
    s.pushHistorique();
    emit salleMiseAJour(id);
}

void MqttSource::envoyerConfig(const Salle& cfg)
{
    QJsonObject hor;
    hor.insert("debut", cfg.horaireDebut);
    hor.insert("fin", cfg.horaireFin);
    QJsonObject obj;
    obj.insert("nom", cfg.nom);
    obj.insert("capacite", cfg.capacite);
    obj.insert("horaires", hor);
    m_client.publish(QString("salle/%1/config/set").arg(cfg.id),
                     QJsonDocument(obj).toJson(QJsonDocument::Compact));
    emit logAppend("CONFIG envoyée  salle/" + cfg.id + "/config/set");
}

void MqttSource::commanderTest(const QString& salleId, const QString& composant,
                               const QString& valeur)
{
    QJsonObject obj;
    obj.insert("composant", composant);
    if (composant == "lcd") {
        obj.insert("ligne1", "TEST LCD -> OK");
        obj.insert("ligne2", valeur);
    } else {
        obj.insert("valeur", valeur);
    }
    m_client.publish(QString("salle/%1/test").arg(salleId),
                     QJsonDocument(obj).toJson(QJsonDocument::Compact));
    emit logAppend("TEST envoyé  salle/" + salleId + "/test");
}

void MqttSource::forcerEvacuation(const QString& salleId, bool actif)
{
    QJsonObject obj;
    obj.insert("active", actif);
    m_client.publish(QString("salle/%1/evacuation/force").arg(salleId),
                     QJsonDocument(obj).toJson(QJsonDocument::Compact));
    emit logAppend("ÉVACUATION FORCÉE envoyée  salle/" + salleId);
}

void MqttSource::resetAlertesSalle(const QString& salleId)
{
    for (auto it = m_alertes.begin(); it != m_alertes.end(); ++it) {
        if (it->salleId == salleId && !it->acquittee) {
            it->acquittee = true;
            emit alerteModifiee(it.value());
        }
    }
    QJsonObject obj;
    obj.insert("type", "all");
    m_client.publish(QString("salle/%1/alerte/reset").arg(salleId),
                     QJsonDocument(obj).toJson(QJsonDocument::Compact));
}
