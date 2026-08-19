#include "AmiClient.h"

#include <QDateTime>
#include <QDebug>

constexpr int RECONNECT_INTERVAL_MS = 10000;
constexpr int KEEPALIVE_INTERVAL_MS = 30000;

AmiClient::AmiClient(const Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &AmiClient::surConnecte);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &AmiClient::surErreurSocket);
    connect(m_socket, &QTcpSocket::readyRead, this, &AmiClient::surPretLecture);
    connect(m_socket, &QTcpSocket::disconnected, this, &AmiClient::surDeconnecte);

    m_timerReconnexion = new QTimer(this);
    m_timerReconnexion->setInterval(RECONNECT_INTERVAL_MS);
    connect(m_timerReconnexion, &QTimer::timeout, this, &AmiClient::verifierConnexion);

    m_timerKeepalive = new QTimer(this);
    m_timerKeepalive->setInterval(KEEPALIVE_INTERVAL_MS);
    connect(m_timerKeepalive, &QTimer::timeout, this, [this]() {
        if (m_connecte && m_authentifie) {
            envoyerCommande("Action: Ping\r\nActionID: keepalive\r\n\r\n");
        }
    });

    verifierConnexion();
    m_timerReconnexion->start();
}

AmiClient::~AmiClient()
{
    m_timerReconnexion->stop();
    m_timerKeepalive->stop();
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
        m_socket->waitForDisconnected(1000);
    }
}

void AmiClient::verifierConnexion()
{
    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        m_socket->connectToHost(m_config.host, m_config.port);
    }
}

void AmiClient::surConnecte()
{
    // Lire la bannière de bienvenue
    m_buffer.clear();
    m_authentifie = false;

    // Envoyer login
    QByteArray login;
    login.append("Action: Login\r\n");
    login.append("Username: ").append(m_config.user.toUtf8()).append("\r\n");
    login.append("Secret: ").append(m_config.password.toUtf8()).append("\r\n");
    login.append("Events: off\r\n\r\n");
    envoyerCommande(login);
}

void AmiClient::surErreurSocket(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    if (m_connecte) {
        m_connecte = false;
        m_authentifie = false;
        emit connexionChangee(false);
        emit erreur(QString("Erreur socket AMI: %1").arg(m_socket->errorString()));
    }
}

void AmiClient::surPretLecture()
{
    m_buffer.append(m_socket->readAll());

    // Analyser les réponses complètes (séparées par double \r\n)
    while (true) {
        int pos = m_buffer.indexOf("\r\n\r\n");
        if (pos < 0)
            break;

        QByteArray reponse = m_buffer.left(pos + 4);
        m_buffer.remove(0, pos + 4);
        analyserReponse(reponse);
    }
}

void AmiClient::analyserReponse(const QByteArray& data)
{
    // Parser les lignes clé: valeur
    QMap<QString, QString> champs;
    for (const QByteArray& ligne : data.split('\n')) {
        int idx = ligne.indexOf(':');
        if (idx > 0) {
            QString cle = ligne.left(idx).trimmed();
            QString valeur = ligne.mid(idx + 1).trimmed();
            champs[cle] = valeur;
        }
    }

    const QString response = champs.value("Response");
    const QString actionId = champs.value("ActionID");

    if (response == "Success") {
        if (actionId == "keepalive") {
            // OK
        } else if (!m_actionIdEnCours.isEmpty() && actionId == m_actionIdEnCours) {
            m_actionIdEnCours.clear();
        } else if (champs.contains("Message") && champs["Message"].contains("Authentication accepted")) {
            m_authentifie = true;
            if (!m_connecte) {
                m_connecte = true;
                m_timerKeepalive->start();
                m_timerReconnexion->stop();
                emit connexionChangee(true);
            }
        } else if (actionId.startsWith("msg_")) {
            // Réponse à MessageSend
            emit messageEnvoye("", true, "MessageSend OK");
        }
    } else if (response == "Error") {
        const QString errorMsg = champs.value("Message");
        if (actionId.startsWith("msg_")) {
            emit messageEnvoye("", false, errorMsg);
        } else if (champs.contains("Message") && champs["Message"].contains("Authentication failed")) {
            emit erreur("AMI authentification échouée: " + errorMsg);
        }
    }
}

void AmiClient::surDeconnecte()
{
    if (m_connecte) {
        m_connecte = false;
        m_authentifie = false;
        m_timerKeepalive->stop();
        emit connexionChangee(false);
    }
    if (!m_timerReconnexion->isActive()) {
        m_timerReconnexion->start();
    }
}

void AmiClient::envoyerCommande(const QByteArray& commande)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(commande);
        m_socket->flush();
    } else {
        QMutexLocker lock(&m_mutex);
        m_fileCommandes.enqueue(commande);
    }
}

void AmiClient::envoyerProchain()
{
    QMutexLocker lock(&m_mutex);
    while (!m_fileCommandes.isEmpty() && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(m_fileCommandes.dequeue());
    }
    m_socket->flush();
}

bool AmiClient::envoyerMessage(const QString& extension, const QString& corpsMessage)
{
    if (!m_connecte || !m_authentifie) {
        emit erreur("AMI non connecté ou non authentifié");
        return false;
    }

    if (extension.isEmpty() || corpsMessage.isEmpty()) {
        emit erreur("Paramètres invalides pour MessageSend");
        return false;
    }

    const QString actionId = QString("msg_%1").arg(QDateTime::currentMSecsSinceEpoch());
    m_actionIdEnCours = actionId;

    QByteArray cmd;
    cmd.append("Action: MessageSend\r\n");
    cmd.append("ActionID: ").append(actionId.toUtf8()).append("\r\n");
    cmd.append("To: ").append(QString("pjsip:%1").arg(extension).toUtf8()).append("\r\n");
    cmd.append("From: i++\r\n");
    cmd.append("Body: ").append(corpsMessage.toUtf8()).append("\r\n");
    cmd.append("\r\n");

    envoyerCommande(cmd);
    return true;
}