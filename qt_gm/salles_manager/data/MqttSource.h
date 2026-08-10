#pragma once

#include <QTimer>
#include <QVector>

#include "data/DataSource.h"
#include "data/MqttClient.h"

class DensiteEstimator;

class MqttSource : public DataSource
{
    Q_OBJECT

public:
    explicit MqttSource(QObject* parent = nullptr);
    ~MqttSource() override;

    void start() override;
    void stop() override;
    QString type() const override { return QStringLiteral("mqtt"); }

    void connecter(const QString& host, quint16 port, const QString& clientId);
    void creerSalle(const Salle& salle) override;
    void modifierSalle(const Salle& salle) override;
    void supprimerSalle(const QString& id) override;
    void getHauteurPorte(const QString& salleId) override;
    void actualiserSalle(const QString& salleId) override;

signals:
    void connexionDemandee();

private slots:
    void onMessage(const QString& topic, const QByteArray& payload);
    void onMqttState(MqttClient::State state);
    void onMqttError(const QString& message);
    void onMesureTimeout();
    void onWatchdogTimeout();

private:
    void publierConfiguration(const Salle& salle);
    void finaliserMesure(bool succes, const QString& note);
    void preparerDensite(const QString& salleId);
    void verifierFluxSortie(const QString& salleId);
    static double mediane(QVector<double> valeurs);

    MqttClient m_client;
    QTimer m_mesureTimer;
    QTimer m_watchdogTimer;
    QString m_mesureId;
    QVector<double> m_mesuresMm;
    QHash<QString, QVector<qint64>> m_departTimes;
    QHash<QString, DensiteEstimator*> m_densite;
};
