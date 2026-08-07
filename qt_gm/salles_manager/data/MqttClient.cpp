#include "MqttClient.h"

#include <QTcpSocket>
#include <QTimer>

namespace {
QByteArray encodeString(const QString& value)
{
    const QByteArray utf8 = value.toUtf8();
    QByteArray result;
    result.append(char((utf8.size() >> 8) & 0xFF));
    result.append(char(utf8.size() & 0xFF));
    result.append(utf8);
    return result;
}
}

MqttClient::MqttClient(QObject* parent)
    : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &MqttClient::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &MqttClient::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &MqttClient::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
                emit errorMessage(m_socket->errorString());
            });

    m_pingTimer = new QTimer(this);
    m_pingTimer->setInterval(m_keepAlive * 1000 / 2);
    connect(m_pingTimer, &QTimer::timeout, this, &MqttClient::onPingTimeout);
}

void MqttClient::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

void MqttClient::connectToHost()
{
    if (m_hostname.trimmed().isEmpty()) {
        emit errorMessage(QStringLiteral("Adresse du broker vide."));
        return;
    }

    m_rx.clear();
    m_socket->abort();
    setState(Connecting);
    m_socket->connectToHost(m_hostname, m_port);
}

void MqttClient::disconnectFromHost()
{
    if (m_state == Connected)
        m_socket->write(QByteArray::fromHex("e000"));
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
    body.append(encodeString(QStringLiteral("MQTT")));
    body.append(char(4));
    body.append(char(0x02));
    body.append(char(m_keepAlive >> 8));
    body.append(char(m_keepAlive & 0xFF));
    body.append(encodeString(m_clientId));

    QByteArray header;
    header.append(char(0x10));
    encodeRemainingLength(body.size(), header);
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

        const quint8 first = quint8(m_rx.at(0));
        const quint8 type = first >> 4;
        const quint8 flags = first & 0x0F;
        int position = 1;
        const int remaining = decodeRemainingLength(m_rx, position);
        if (remaining < 0 || position + remaining > m_rx.size())
            return;

        const QByteArray body = m_rx.mid(position, remaining);
        m_rx.remove(0, position + remaining);
        handlePacket(type, flags, body);
    }
}

void MqttClient::handlePacket(quint8 type, quint8 flags, const QByteArray& body)
{
    Q_UNUSED(flags)

    switch (type) {
    case 2: {
        if (body.size() < 2)
            return;
        const quint8 returnCode = quint8(body.at(1));
        m_pingTimer->stop();
        if (returnCode == 0) {
            setState(Connected);
            m_pingTimer->start();
        } else {
            emit errorMessage(QStringLiteral("CONNACK refusé, code %1").arg(returnCode));
            m_socket->disconnectFromHost();
        }
        break;
    }
    case 3: {
        if (body.size() < 2)
            return;
        const quint16 topicLength = (quint8(body.at(0)) << 8) | quint8(body.at(1));
        if (body.size() < 2 + topicLength)
            return;
        const QString topic = QString::fromUtf8(body.mid(2, topicLength));
        const quint8 qos = (flags >> 1) & 0x03;
        int payloadPosition = 2 + topicLength;
        if (qos > 0)
            payloadPosition += 2;
        if (payloadPosition <= body.size())
            emit messageReceived(topic, body.mid(payloadPosition));
        break;
    }
    case 13:
    case 9:
    default:
        break;
    }
}

int MqttClient::decodeRemainingLength(const QByteArray& data, int& position)
{
    int multiplier = 1;
    int value = 0;
    int bytes = 0;
    while (true) {
        if (position >= data.size())
            return -1;
        const quint8 digit = quint8(data.at(position++));
        value += (digit & 0x7F) * multiplier;
        if ((digit & 0x80) == 0)
            return value;
        multiplier *= 128;
        if (++bytes >= 4)
            return -1;
    }
}

void MqttClient::encodeRemainingLength(int length, QByteArray& output)
{
    do {
        quint8 digit = quint8(length % 128);
        length /= 128;
        if (length > 0)
            digit |= 0x80;
        output.append(char(digit));
    } while (length > 0);
}

bool MqttClient::subscribe(const QString& topic, quint8 qos)
{
    if (m_state != Connected)
        return false;

    const quint16 packetId = m_packetId++;
    QByteArray body;
    body.append(char(packetId >> 8));
    body.append(char(packetId & 0xFF));
    body.append(encodeString(topic));
    body.append(char(qos & 0x03));

    QByteArray header;
    header.append(char(0x82));
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
        const quint16 packetId = m_packetId++;
        body.append(char(packetId >> 8));
        body.append(char(packetId & 0xFF));
    }
    body.append(payload);

    QByteArray header;
    header.append(char(0x30 | ((qos & 0x03) << 1) | (retain ? 1 : 0)));
    encodeRemainingLength(body.size(), header);
    sendPacket(header, body);
    return true;
}

void MqttClient::onPingTimeout()
{
    if (m_state == Connected)
        m_socket->write(QByteArray::fromHex("c000"));
}

void MqttClient::onDisconnected()
{
    m_pingTimer->stop();
    m_rx.clear();
    setState(Disconnected);
}
