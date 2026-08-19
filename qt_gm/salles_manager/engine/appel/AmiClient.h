#pragma once

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>
#include <QQueue>
#include <QMutex>

class AmiClient : public QObject
{
    Q_OBJECT
public:
    struct Config {
        QString host = "127.0.0.1";
        quint16 port = 5038;
        QString user = "supervision";
        QString password = "i_plus_plus";
    };

    explicit AmiClient(const Config& config, QObject* parent = nullptr);
    ~AmiClient() override;

    bool estConnecte() const { return m_connecte; }
    void setConfig(const Config& config) { m_config = config; }

    // Envoie un SIP MESSAGE à une extension PJSIP
    // Retourne true si la commande a été envoyée (réponse asynchrone via messageEnvoye)
    bool envoyerMessage(const QString& extension, const QString& corpsMessage);

signals:
    void connexionChangee(bool connecte);
    void messageEnvoye(const QString& extension, bool succes, const QString& details);
    void erreur(const QString& message);

private slots:
    void surConnecte();
    void surErreurSocket(QAbstractSocket::SocketError);
    void surPretLecture();
    void surDeconnecte();
    void envoyerProchain();
    void verifierConnexion();

private:
    void analyserReponse(const QByteArray& data);
    void envoyerCommande(const QByteArray& commande);

    Config m_config;
    QTcpSocket* m_socket = nullptr;
    QTimer* m_timerReconnexion = nullptr;
    QTimer* m_timerKeepalive = nullptr;
    bool m_connecte = false;
    bool m_authentifie = false;
    QByteArray m_buffer;
    QQueue<QByteArray> m_fileCommandes;
    QMutex m_mutex;
    QString m_actionIdEnCours;
};

struct AmiAction {
    QString action;
    QMap<QString, QString> params;
};