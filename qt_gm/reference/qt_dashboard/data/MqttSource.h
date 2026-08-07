#pragma once

#include <QDateTime>

#include "DataSource.h"
#include "MqttClient.h"

class MqttSource : public DataSource
{
    Q_OBJECT

public:
    explicit MqttSource(QObject* parent = nullptr);

    void start() override;
    void stop() override;
    QString type() const override { return "mqtt"; }

    void connecter(const QString& host, quint16 port, const QString& clientId);

    void envoyerConfig(const Salle& s) override;
    void commanderTest(const QString& salleId, const QString& composant,
                       const QString& valeur) override;
    void forcerEvacuation(const QString& salleId, bool actif) override;
    void resetAlertesSalle(const QString& salleId) override;

private slots:
    void onEtatMqtt(MqttClient::State state);
    void onMessage(const QString& topic, const QByteArray& payload);

private:
    void majSalle(const QString& id, quint64 maintenantMs);

    MqttClient m_client;
    QString m_clientId;
    QHash<QString, quint64> m_derniereMaj; // throttle 1 Hz par salle
};
