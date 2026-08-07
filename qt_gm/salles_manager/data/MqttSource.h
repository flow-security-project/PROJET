#pragma once

#include <QTimer>
#include <QVector>

#include "data/DataSource.h"
#include "data/MqttClient.h"

class MqttSource : public DataSource
{
    Q_OBJECT

public:
    explicit MqttSource(QObject* parent = nullptr);

    void start() override;
    void stop() override;
    QString type() const override { return QStringLiteral("mqtt"); }

    void connecter(const QString& host, quint16 port, const QString& clientId);
    void creerSalle(const Salle& salle) override;
    void modifierSalle(const Salle& salle) override;
    void getHauteurPorte(const QString& salleId) override;
    void actualiserSalle(const QString& salleId) override;

signals:
    void connexionDemandee();

private slots:
    void onMessage(const QString& topic, const QByteArray& payload);
    void onMqttState(MqttClient::State state);
    void onMqttError(const QString& message);
    void onMesureTimeout();

private:
    void publierConfiguration(const Salle& salle);
    void finaliserMesure(bool succes, const QString& note);
    void ajouterNoeudDecouvert(const QString& id);
    static double mediane(QVector<double> valeurs);

    MqttClient m_client;
    QTimer m_mesureTimer;
    QString m_mesureId;
    QVector<double> m_mesuresMm;
};
