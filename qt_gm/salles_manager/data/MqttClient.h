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
        Connected = 2
    };
    Q_ENUM(State)

    explicit MqttClient(QObject* parent = nullptr);

    void setHostname(const QString& hostname) { m_hostname = hostname; }
    void setPort(quint16 port) { m_port = port; }
    void setClientId(const QString& clientId) { m_clientId = clientId; }

    State state() const { return m_state; }

    void connectToHost();
    void disconnectFromHost();
    bool subscribe(const QString& topic, quint8 qos = 0);
    bool publish(const QString& topic, const QByteArray& payload,
                 quint8 qos = 0, bool retain = false);

signals:
    void stateChanged(MqttClient::State state);
    void errorMessage(const QString& message);
    void messageReceived(const QString& topic, const QByteArray& payload);

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onPingTimeout();

private:
    void setState(State state);
    void sendPacket(const QByteArray& header, const QByteArray& body);
    void processRx();
    void handlePacket(quint8 type, quint8 flags, const QByteArray& body);
    static int decodeRemainingLength(const QByteArray& data, int& position);
    static void encodeRemainingLength(int length, QByteArray& output);

    QTcpSocket* m_socket = nullptr;
    QTimer* m_pingTimer = nullptr;
    QString m_hostname;
    quint16 m_port = 1883;
    QString m_clientId;
    State m_state = Disconnected;
    QByteArray m_rx;
    quint16 m_packetId = 1;
    int m_keepAlive = 60;
};
