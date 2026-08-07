#pragma once

#include "engine/GmSource.h"
#include "engine/mqtt/MqttClient.h"

class MqttGmSource : public GmSource
{
    Q_OBJECT

public:
    explicit MqttGmSource(QObject* parent = nullptr);

    void start() override;
    void stop() override;
    QString type() const override { return "mqtt"; }

    // Topics attendus du binôme :
    //   salle/{id}/config   -> {"nom":"...","capacite":30,"horaires":{"debut":"07:00","fin":"22:00"}}
    //   salle/{id}/etat     -> {"occupation":24,"densite":0.8,"en_ligne":true}
    //   salle/{id}/passage  -> {"direction":"entree"|"sortie"}
    void connecter(const QString& host, quint16 port, const QString& clientId);
    void deconnecter();

private slots:
    void onMessage(const QString& topic, const QByteArray& payload);
    void onMqttState(MqttClient::State state);

private:
    void traiterConfig(const QString& id, const QByteArray& payload);
    void traiterEtat(const QString& id, const QByteArray& payload);
    void traiterPassage(const QString& id, const QByteArray& payload);

    MqttClient m_client;
};
