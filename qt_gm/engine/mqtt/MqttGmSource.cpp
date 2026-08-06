#include "MqttGmSource.h"

#include <QJsonDocument>
#include <QJsonObject>

MqttGmSource::MqttGmSource(QObject* parent)
    : GmSource(parent)
{
    connect(&m_client, &MqttClient::messageReceived,
            this, &MqttGmSource::onMessage);
    connect(&m_client, &MqttClient::stateChanged,
            this, &MqttGmSource::onMqttState);
}

void MqttGmSource::start()
{
    logger("Source MQTT prête — en attente du binôme (config/etat/passage)");
    emit statutMqtt(false, "Déconnecté");
}

void MqttGmSource::stop()
{
    m_client.disconnectFromHost();
}

void MqttGmSource::connecter(const QString& host, quint16 port, const QString& clientId)
{
    m_client.setHostname(host);
    m_client.setPort(port);
    m_client.setClientId(clientId);
    m_client.connectToHost();
}

void MqttGmSource::deconnecter()
{
    m_client.disconnectFromHost();
}

void MqttGmSource::onMqttState(MqttClient::State state)
{
    switch (state) {
    case MqttClient::Connected:
        m_client.subscribe("salle/+/config");
        m_client.subscribe("salle/+/etat");
        m_client.subscribe("salle/+/passage");
        emit statutMqtt(true, "Connecté");
        logger("MQTT connecté — abonné à salle/+/config, etat, passage");
        break;
    case MqttClient::Disconnected:
        emit statutMqtt(false, "Déconnecté");
        break;
    default:
        break;
    }
}

void MqttGmSource::onMessage(const QString& topic, const QByteArray& payload)
{
    // salle/{id}/categorie
    const QStringList parts = topic.split('/');
    if (parts.size() < 3 || parts.at(0) != "salle")
        return;
    const QString id = parts.at(1);
    const QString categorie = parts.at(2);

    if (categorie == "config")
        traiterConfig(id, payload);
    else if (categorie == "etat")
        traiterEtat(id, payload);
    else if (categorie == "passage")
        traiterPassage(id, payload);
}

void MqttGmSource::traiterConfig(const QString& id, const QByteArray& payload)
{
    const QJsonObject o = QJsonDocument::fromJson(payload).object();
    if (o.isEmpty())
        return;
    SalleGm s;
    s.id = id;
    s.nom = o.value("nom").toString(id);
    s.capacite = o.value("capacite").toInt(0);
    const QJsonObject h = o.value("horaires").toObject();
    s.horaireDebut = h.value("debut").toString("");
    s.horaireFin = h.value("fin").toString("");
    s.enLigne = false;
    creerSalle(s);
    logger(QString("CONFIG  salle/%1  %2  cap %3").arg(id, s.nom).arg(s.capacite));
}

void MqttGmSource::traiterEtat(const QString& id, const QByteArray& payload)
{
    const QJsonObject o = QJsonDocument::fromJson(payload).object();
    if (o.isEmpty())
        return;
    mettreAjourEtat(id,
                    o.value("occupation").toInt(-1),
                    o.value("densite").toDouble(0.0),
                    o.value("en_ligne").toBool(true));
}

void MqttGmSource::traiterPassage(const QString& id, const QByteArray& payload)
{
    const QJsonObject o = QJsonDocument::fromJson(payload).object();
    if (o.isEmpty())
        return;
    const QString direction = o.value("direction").toString();
    compterPassage(id, direction);
    if (direction == "entree" || direction == "sortie")
        logger(QString("PASSAGE  salle/%1  %2").arg(id, direction));
}
