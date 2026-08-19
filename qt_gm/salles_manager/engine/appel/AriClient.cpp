#include "AriClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QUuid>
#include <QDebug>

constexpr int POLL_INTERVAL_MS = 1000;
constexpr int MAX_POLL_TENTATIVES = 120; // 2 minutes max
constexpr int CONNEXION_TIMEOUT_MS = 3000;
constexpr int CONNEXION_CHECK_INTERVAL_MS = 10000;

AriClient::AriClient(const Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished, this, &AriClient::surReponse);

    m_baseUrl = QUrl(QString("http://%1:%2/ari").arg(config.host).arg(config.port));

    // Header Basic Auth
    QByteArray credentials = QString("%1:%2").arg(config.user, config.password).toUtf8();
    m_authHeader = "Basic " + credentials.toBase64();

    m_timerConnexion = new QTimer(this);
    m_timerConnexion->setInterval(CONNEXION_CHECK_INTERVAL_MS);
    connect(m_timerConnexion, &QTimer::timeout, this, &AriClient::verifierConnexion);

    m_timerPolls = new QTimer(this);
    m_timerPolls->setInterval(POLL_INTERVAL_MS);
    connect(m_timerPolls, &QTimer::timeout, this, &AriClient::pollerAppelsEnCours);

    verifierConnexion();
    m_timerConnexion->start();
    m_timerPolls->start();
}

AriClient::~AriClient()
{
    m_timerConnexion->stop();
    m_timerPolls->stop();
}

QNetworkRequest AriClient::creerRequete(const QString& chemin) const
{
    QUrl url = m_baseUrl.resolved(QUrl(chemin));
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", m_authHeader.toUtf8());
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Accept", "application/json");
    return request;
}

void AriClient::verifierConnexion()
{
    QNetworkRequest req = creerRequete("/asterisk/info");
    QNetworkReply* reply = m_nam->get(req);
    reply->setProperty("action", "check_connexion");
}

void AriClient::surReponse(QNetworkReply* reply)
{
    const QString action = reply->property("action").toString();
    const QString channelId = reply->property("channelId").toString();

    if (reply->error() != QNetworkReply::NoError) {
        if (action == "check_connexion") {
            if (m_connecte) {
                m_connecte = false;
                emit connexionChangee(false);
            }
        } else if (!channelId.isEmpty()) {
            QMutexLocker lock(&m_mutex);
            if (m_appels.contains(channelId)) {
                auto& appel = m_appels[channelId];
                if (appel.info.etat != "termine" && appel.info.etat != "echoue") {
                    mettreAJourAppel(channelId, "echoue");
                }
            }
        }
        if (action != "check_connexion" && action != "poll") {
            emit erreur(QString("ARI %1: %2").arg(action, reply->errorString()));
        }
        reply->deleteLater();
        return;
    }

    const QByteArray data = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (action == "check_connexion") {
        if (!m_connecte && statusCode == 200) {
            m_connecte = true;
            emit connexionChangee(true);
        }
    } else if (action == "originate") {
        if (statusCode == 200 || statusCode == 201) {
            const QString channelId = extraireChannelId(data);
            if (!channelId.isEmpty()) {
                QMutexLocker lock(&m_mutex);
                if (m_appels.contains(channelId)) {
                    m_appels[channelId].info.etat = "sonne";
                    emit appelChange(m_appels[channelId].info);
                }
            }
        } else {
            emit erreur(QString("Échec originate: %1").arg(data));
        }
    } else if (action == "answer") {
        // Pas d'action spéciale, le poll détectera le changement
    } else if (action == "play") {
        // Pas d'action spéciale
    } else if (action == "hangup") {
        QMutexLocker lock(&m_mutex);
        if (m_appels.contains(channelId)) {
            m_appels[channelId].info.etat = "termine";
            emit appelChange(m_appels[channelId].info);
        }
    } else if (action == "poll") {
        if (statusCode == 200) {
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                QString state = obj.value("state").toString("Up");
                QString channelId = obj.value("id").toString();

                QMutexLocker lock(&m_mutex);
                if (m_appels.contains(channelId)) {
                    auto& appel = m_appels[channelId];
                    QString nouvelEtat = state;
                    if (state == "Up" || state == "Ring" || state == "Ringing")
                        nouvelEtat = "en_cours";
                    else if (state == "Down" || state == "Hangup" || state == "Destroyed")
                        nouvelEtat = "termine";

                    if (appel.info.etat != nouvelEtat) {
                        mettreAJourAppel(channelId, nouvelEtat);
                    }
                }
            }
        } else if (statusCode == 404) {
            // Canal détruit = appel terminé
            QMutexLocker lock(&m_mutex);
            if (m_appels.contains(channelId)) {
                auto& appel = m_appels[channelId];
                if (appel.info.etat != "termine" && appel.info.etat != "echoue") {
                    mettreAJourAppel(channelId, "termine");
                }
            }
        }
    }

    reply->deleteLater();
}

QString AriClient::appeler(const QString& extension, const QString& wavFileName)
{
    if (!m_connecte) {
        emit erreur("ARI non connecté");
        return QString();
    }

    if (extension.isEmpty() || wavFileName.isEmpty()) {
        emit erreur("Paramètres invalides pour l'appel");
        return QString();
    }

    // Construire le JSON pour originate
    QJsonObject body;
    body["endpoint"] = QString("PJSIP/%1").arg(extension);
    body["extension"] = "s";
    body["context"] = "mes-amis";
    body["priority"] = 1;
    body["app"] = m_config.appName;
    body["appArgs"] = wavFileName; // nom du fichier WAV sans chemin (dans /var/lib/asterisk/sounds/)
    body["callerId"] = QString("i++ <%1>").arg(extension);
    body["timeout"] = 30; // secondes

    QJsonDocument doc(body);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    QString channelId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Enregistrer l'appel en attente
    {
        QMutexLocker lock(&m_mutex);
        AppelEnCours appel;
        appel.info.channelId = channelId;
        appel.info.etat = "init";
        appel.info.extension = extension;
        appel.info.wavFile = wavFileName;
        appel.info.debut = QDateTime::currentDateTime();
        m_appels.insert(channelId, appel);
    }

    // Envoyer la requête originate
    envoyerRequete("POST", "/channels", jsonData, channelId, "originate");

    return channelId;
}

void AriClient::raccrocher(const QString& channelId)
{
    if (channelId.isEmpty())
        return;

    envoyerRequete("DELETE", QString("/channels/%1").arg(channelId), QByteArray(), channelId, "hangup");
}

void AriClient::actualiserStatut(const QString& channelId)
{
    if (channelId.isEmpty() || !m_connecte)
        return;
    envoyerRequete("GET", QString("/channels/%1").arg(channelId), QByteArray(), channelId, "poll");
}

void AriClient::envoyerRequete(const QString& methode, const QString& chemin,
                               const QByteArray& body,
                               const QString& channelId,
                               const QString& action)
{
    QNetworkRequest req = creerRequete(chemin);
    QNetworkReply* reply = nullptr;

    if (methode == "GET") {
        reply = m_nam->get(req);
    } else if (methode == "POST") {
        reply = m_nam->post(req, body);
    } else if (methode == "DELETE") {
        reply = m_nam->deleteResource(req);
    } else {
        return;
    }

    reply->setProperty("action", action);
    if (!channelId.isEmpty())
        reply->setProperty("channelId", channelId);
}

QString AriClient::extraireChannelId(const QByteArray& json) const
{
    QJsonDocument doc = QJsonDocument::fromJson(json);
    if (doc.isNull() || !doc.isObject())
        return QString();
    return doc.object().value("id").toString();
}

void AriClient::mettreAJourAppel(const QString& channelId, const QString& etat)
{
    if (!m_appels.contains(channelId))
        return;

    auto& appel = m_appels[channelId];
    QString ancienEtat = appel.info.etat;
    appel.info.etat = etat;

    if (etat == "en_cours" || etat == "sonne") {
        emit appelChange(appel.info);
    } else if (etat == "termine" || etat == "echoue") {
        emit appelChange(appel.info);
        emit appelTermine(channelId, etat == "termine");
        // Nettoyer après un délai pour permettre les derniers polls
        QTimer::singleShot(5000, this, [this, channelId]() {
            QMutexLocker lock(&m_mutex);
            m_appels.remove(channelId);
        });
    }
}

void AriClient::pollerAppelsEnCours()
{
    if (!m_connecte)
        return;

    QList<QString> channels;
    {
        QMutexLocker lock(&m_mutex);
        for (auto it = m_appels.begin(); it != m_appels.end(); ++it) {
            const auto& appel = it.value();
            if (appel.info.etat != "termine" && appel.info.etat != "echoue") {
                channels.append(it.key());
                it->tentativesPoll++;
            }
        }
    }

    for (const QString& channelId : channels) {
        actualiserStatut(channelId);
    }

    // Nettoyer les appels bloqués trop longtemps
    {
        QMutexLocker lock(&m_mutex);
        for (auto it = m_appels.begin(); it != m_appels.end();) {
            if (it->tentativesPoll > MAX_POLL_TENTATIVES) {
                if (it->info.etat != "termine" && it->info.etat != "echoue") {
                    emit appelTermine(it.key(), false);
                }
                it = m_appels.erase(it);
            } else {
                ++it;
            }
        }
    }
}