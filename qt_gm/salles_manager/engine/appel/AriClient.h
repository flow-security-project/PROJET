#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QTimer>
#include <QMutex>

class AriClient : public QObject
{
    Q_OBJECT
public:
    struct Config {
        QString host = "127.0.0.1";
        quint16 port = 8088;
        QString user = "supervision";
        QString password = "i_plus_plus";
        QString appName = "supervision";
    };

    struct AppelInfo {
        QString channelId;
        QString etat;          // "en_cours" | "termine" | "echoue" | "sonne"
        QString extension;
        QString wavFile;       // nom du fichier WAV joué (sans chemin)
        QDateTime debut;
    };

    explicit AriClient(const Config& config, QObject* parent = nullptr);
    ~AriClient() override;

    bool estConnecte() const { return m_connecte; }
    Config config() const { return m_config; }
    void setConfig(const Config& config) { m_config = config; }

    // Lance un appel vers une extension SIP (ex: "1001")
    // Joue le fichier WAV (nom de base, ex: "tts_123.wav") depuis /var/lib/asterisk/sounds/
    // Retourne channelId si lancé, chaîne vide si échec
    QString appeler(const QString& extension, const QString& wavFileName);

    // Raccroche un appel en cours
    void raccrocher(const QString& channelId);

    // Force le statut d'un appel (polling interne)
    void actualiserStatut(const QString& channelId);

signals:
    void connexionChangee(bool connecte);
    void appelChange(const AppelInfo& info);
    void appelTermine(const QString& channelId, bool succes);
    void erreur(const QString& message);

private slots:
    void surReponse(QNetworkReply* reply);
    void verifierConnexion();
    void pollerAppelsEnCours();

private:
    void envoyerRequete(const QString& methode, const QString& chemin,
                        const QByteArray& body = QByteArray(),
                        const QString& channelId = QString(),
                        const QString& action = QString());

    QNetworkRequest creerRequete(const QString& chemin) const;
    QString extraireChannelId(const QByteArray& json) const;
    void mettreAJourAppel(const QString& channelId, const QString& etat);

    Config m_config;
    QNetworkAccessManager* m_nam = nullptr;
    QTimer* m_timerConnexion = nullptr;
    QTimer* m_timerPolls = nullptr;
    bool m_connecte = false;
    QString m_authHeader;
    QUrl m_baseUrl;

    struct AppelEnCours {
        AppelInfo info;
        int tentativesPoll = 0;
    };
    QMutex m_mutex;
    QMap<QString, AppelEnCours> m_appels; // channelId -> info
};