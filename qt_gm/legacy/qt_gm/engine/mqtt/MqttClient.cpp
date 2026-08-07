#include "MqttClient.h"

#include <QTcpSocket>
#include <QTimer>

namespace {
QByteArray encodeString(const QString& s)
{
    const QByteArray utf8 = s.toUtf8();
    QByteArray out;
    out.append(char(utf8.size() >> 8));
    out.append(char(utf8.size() & 0xFF));
    out.append(utf8);
    return out;
}
} // namespace

MqttClient::MqttClient(QObject* parent)
    : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &MqttClient::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &MqttClient::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &MqttClient::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError e) {
        if (e == QAbstractSocket::ConnectionRefusedError
            || e == QAbstractSocket::HostNotFoundError
            || e == QAbstractSocket::RemoteHostClosedError)
            setError(TransportInvalid);
    });

    m_pingTimer = new QTimer(this);
    m_pingTimer->setInterval(m_keepAlive * 1000 / 2);
    connect(m_pingTimer, &QTimer::timeout, this, &MqttClient::onPingTimeout);
}

void MqttClient::setState(State s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged(s);
}

void MqttClient::setError(Error e)
{
    if (m_error == e)
        return;
    m_error = e;
    emit errorChanged(e);
}

void MqttClient::connectToHost()
{
    m_rx.clear();
    m_error = NoError;
    setState(Connecting);
    m_socket->connectToHost(m_host, m_port);
}

void MqttClient::disconnectFromHost()
{
    if (m_state == Connected)
        m_socket->write(QByteArray("\xE0\x00", 2)); // DISCONNECT
    m_socket->disconnectFromHost();
}

void MqttClient::sendPacket(const QByteArray& header, const QByteArray& body)
{
    m_socket->write(header);
    m_socket->write(body);
}

void MqttClient::onConnected()
{
    QByteArray body;
    body.append(encodeString("MQTT"));
    body.append(char(4));                 // niveau protocole 3.1.1
    body.append(char(0x02));              // flags : clean session
    body.append(char(m_keepAlive >> 8));
    body.append(char(m_keepAlive & 0xFF));
    body.append(encodeString(m_clientId));

    QByteArray header;
    header.append(char(0x10));            // CONNECT
    const int rem = body.size();
    if (rem < 128)
        header.append(char(rem));
    else
        encodeRemainingLength(rem, header);
    sendPacket(header, body);
}

void MqttClient::onReadyRead()
{
    m_rx.append(m_socket->readAll());
    processRx();
}

void MqttClient::processRx()
{
    while (true) {
        if (m_rx.size() < 2)
            return;
        const quint8 type = quint8(m_rx.at(0)) >> 4;
        int pos = 1;
        const int remLen = decodeRemainingLength(m_rx, pos);
        if (remLen < 0 || pos + remLen > m_rx.size())
            return; // paquet incomplet, on attend la suite
        const QByteArray body = m_rx.mid(pos, remLen);
        m_rx.remove(0, pos + remLen);
        handlePacket(type, body);
    }
}

void MqttClient::handlePacket(quint8 type, const QByteArray& body)
{
    switch (type) {
    case 2: { // CONNACK
        if (body.size() < 2)
            return;
        const quint8 rc = quint8(body.at(1));
        m_pingTimer->stop();
        if (rc == 0) {
            setError(NoError);
            setState(Connected);
            m_pingTimer->start();
        } else {
            const Error e = rc <= 5 ? Error(rc) : UnknownError;
            setError(e);
            m_socket->disconnectFromHost();
        }
        break;
    }
    case 3: { // PUBLISH
        if (body.size() < 2)
            return;
        const quint16 topicLen = (quint8(body.at(0)) << 8) | quint8(body.at(1));
        if (body.size() < 2 + topicLen)
            return;
        const QString topic = QString::fromUtf8(body.mid(2, topicLen));
        const quint8 qos = (quint8(body.at(0)) >> 1) & 0x03;
        int payloadPos = 2 + topicLen;
        if (qos > 0)
            payloadPos += 2; // packet id QoS1/2 ignoré (lecture seule)
        emit messageReceived(topic, body.mid(payloadPos));
        break;
    }
    case 9:  // SUBACK : pas de suivi nécessaire (probe QoS0)
    case 13: // PINGRESP
    default:
        break;
    }
}

int MqttClient::decodeRemainingLength(const QByteArray& data, int& pos)
{
    int multiplier = 1;
    int value = 0;
    int bytes = 0;
    while (true) {
        if (pos >= data.size())
            return -1;
        const quint8 digit = quint8(data.at(pos++));
        value += (digit & 0x7F) * multiplier;
        if ((digit & 0x80) == 0)
            return value;
        multiplier *= 128;
        if (++bytes >= 4)
            return -1;
    }
}

void MqttClient::encodeRemainingLength(int len, QByteArray& out)
{
    do {
        quint8 digit = len % 128;
        len /= 128;
        if (len > 0)
            digit |= 0x80;
        out.append(char(digit));
    } while (len > 0);
}

bool MqttClient::subscribe(const QString& topic, quint8 qos)
{
    if (m_state != Connected)
        return false;
    QByteArray body;
    const quint16 id = m_packetId++;
    body.append(char(id >> 8));
    body.append(char(id & 0xFF));
    body.append(encodeString(topic));
    body.append(char(qos & 0x03));

    QByteArray header;
    header.append(char(0x82)); // SUBSCRIBE
    if (body.size() < 128)
        header.append(char(body.size()));
    else
        encodeRemainingLength(body.size(), header);
    sendPacket(header, body);
    return true;
}

bool MqttClient::publish(const QString& topic, const QByteArray& payload,
                         quint8 qos, bool retain)
{
    if (m_state != Connected)
        return false;
    QByteArray body = encodeString(topic);
    if (qos > 0) {
        const quint16 id = m_packetId++;
        body.append(char(id >> 8));
        body.append(char(id & 0xFF));
    }
    body.append(payload);

    QByteArray header;
    header.append(char(0x30 | ((qos & 0x03) << 1) | (retain ? 1 : 0)));
    if (body.size() < 128)
        header.append(char(body.size()));
    else
        encodeRemainingLength(body.size(), header);
    sendPacket(header, body);
    return true;
}

void MqttClient::onPingTimeout()
{
    if (m_state == Connected)
        m_socket->write(QByteArray("\xC0\x00", 2)); // PINGREQ
}

void MqttClient::onDisconnected()
{
    m_pingTimer->stop();
    m_rx.clear();
    setState(Disconnected);
}
