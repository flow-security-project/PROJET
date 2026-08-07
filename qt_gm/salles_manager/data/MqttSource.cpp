#include "MqttSource.h"

#include <algorithm>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

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
}

void MqttSource::start()
{
    emit statutSource(false, QStringLiteral("Déconnecté"));
    emit logAppend(QStringLiteral("Source MQTT prête — indiquez le broker"));
}

void MqttSource::stop()
{
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
    updated.tendance = previous.tendance;
    updated.nbEntrees = previous.nbEntrees;
    updated.nbSorties = previous.nbSorties;
    updated.occHist = previous.occHist;
    updated.densHist = previous.densHist;
    updated.entHist = previous.entHist;
    updated.sortHist = previous.sortHist;
    updated.enAttente = true;
    m_salles.insert(updated.id, updated);
    emit salleMiseAJour(updated.id);
    publierConfiguration(updated);
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
        if (id == m_mesureId && status != 4 && millimetres > 0.0) {
            m_mesuresMm.append(millimetres);
            if (m_mesuresMm.size() >= 7)
                finaliserMesure(true, QStringLiteral("Mesure ToF confirmée"));
        }
        return;
    }

    if (!m_salles.contains(id))
        return;

    Salle& salle = m_salles[id];
    if (suffix == QStringLiteral("heartbeat")) {
        salle.enLigne = object.value(QStringLiteral("etat")).toString()
                        == QStringLiteral("online");
    } else if (suffix == QStringLiteral("raw/ultrason")) {
        const QString event = object.value(QStringLiteral("event")).toString();
        if (event == QStringLiteral("presence"))
            ++salle.nbEntrees;
        else if (event == QStringLiteral("depart"))
            ++salle.nbSorties;
    } else if (suffix == QStringLiteral("raw/env")) {
        // The current room model keeps the aggregate state; environmental data
        // remains available for the future safety widgets.
    } else if (suffix == QStringLiteral("etat")) {
        salle.occupation = object.value(QStringLiteral("occupation")).toInt(-1);
        salle.capacite = object.value(QStringLiteral("capacite")).toInt(salle.capacite);
        salle.densite = object.value(QStringLiteral("densite")).toDouble(salle.densite);
        salle.enLigne = object.value(QStringLiteral("en_ligne")).toBool(true);
        salle.enAttente = false;
        salle.pushHistorique();
    } else if (suffix == QStringLiteral("config/confirm")) {
        salle.nom = object.value(QStringLiteral("nom")).toString(salle.nom);
        salle.capacite = object.value(QStringLiteral("capacite")).toInt(salle.capacite);
        const QJsonObject horaires = object.value(QStringLiteral("horaires")).toObject();
        salle.horaireDebut = horaires.value(QStringLiteral("debut"))
                                 .toString(salle.horaireDebut);
        salle.horaireFin = horaires.value(QStringLiteral("fin"))
                               .toString(salle.horaireFin);
        if (object.contains(QStringLiteral("hauteurPorte_cm"))) {
            salle.hauteurPorteCm = object.value(QStringLiteral("hauteurPorte_cm"))
                                       .toDouble(salle.hauteurPorteCm);
            salle.hauteurPorteMesuree = salle.hauteurPorteCm > 0.0;
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
        emit salleMiseAJour(id);
    }
    emit hauteurPorteMesuree(id, centimetres, true,
                             QStringLiteral("%1 cm, médiane de %2 mesures")
                                 .arg(centimetres, 0, 'f', 1)
                                 .arg(mesures.size()));
}

double MqttSource::mediane(QVector<double> valeurs)
{
    std::sort(valeurs.begin(), valeurs.end());
    const int milieu = valeurs.size() / 2;
    if (valeurs.size() % 2 == 0)
        return (valeurs.at(milieu - 1) + valeurs.at(milieu)) / 2.0;
    return valeurs.at(milieu);
}
