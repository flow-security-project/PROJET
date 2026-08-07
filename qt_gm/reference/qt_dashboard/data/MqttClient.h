#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class QTcpSocket;
class QTimer;

class MqttClient : public QObject
{
    Q_OBJECT

public:
    enum State {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2,
    };
    Q_ENUM(State)

    enum Error {
        NoError = 0,
        InvalidProtocolVersion = 1,
        IdRejected = 2,
        ServerUnavailable = 3,
        BadUsernameOrPassword = 4,
        NotAuthorized = 5,
        TransportInvalid = 6,
        ProtocolViolation = 7,
        UnknownError = 8,
    };
    Q_ENUM(Error)

    explicit MqttClient(QObject* parent = nullptr);

    void setHostname(const QString& host) { m_host = host; }
    void setPort(quint16 port) { m_port = port; }
    void setClientId(const QString& id) { m_clientId = id; }
    State state() const { return m_state; }
    Error error() const { return m_error; }

    void connectToHost();
    void disconnectFromHost();
    bool subscribe(const QString& topic, quint8 qos = 0);
    bool publish(const QString& topic, const QByteArray& payload,
                 quint8 qos = 0, bool retain = false);

signals:
    void stateChanged(MqttClient::State state);
    void errorChanged(MqttClient::Error error);
    void messageReceived(const QString& topic, const QByteArray& payload);

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onPingTimeout();

private:
    void setState(State s);
    void setError(Error e);
    void sendPacket(const QByteArray& header, const QByteArray& body);
    void processRx();
    void handlePacket(quint8 type, const QByteArray& body);
    static int decodeRemainingLength(const QByteArray& data, int& pos);
    static void encodeRemainingLength(int len, QByteArray& out);

    QTcpSocket* m_socket = nullptr;
    QTimer* m_pingTimer = nullptr;
    QString m_host;
    quint16 m_port = 1883;
    QString m_clientId;
    State m_state = Disconnected;
    Error m_error = NoError;
    QByteArray m_rx;
    quint16 m_packetId = 1;
    int m_keepAlive = 60;
};
