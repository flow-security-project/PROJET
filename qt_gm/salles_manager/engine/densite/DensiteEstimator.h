#pragma once

#include <QPointF>
#include <QString>
#include <QVector>

struct EchantillonToF
{
    double distanceMm = -1.0;
    qint64 tMs = 0;
};

struct DensiteEstimation
{
    int nbPersonnes = -1;
    double surface = 0.0;
    QString regime;         // "clustering" | "surface"
    double confiance = -1.0;
    int nbClusters = 0;
    bool bascule = false;
};

class DensiteEstimator
{
public:
    DensiteEstimator();

    void setHauteurPorteCm(double centimetres);
    void setTableCalibration(const QVector<QPointF>& table);
    bool chargerCalibration(const QString& cheminJson);

    void ajouterEchantillon(double distanceMm, qint64 tMs);
    void reset();

    DensiteEstimation estimer(qint64 maintenantMs);

private:
    void filtrer();
    double closeness(double distanceMm) const;
    double surfaceOccupee(qint64 maintenantMs) const;
    int compterClusters(qint64 maintenantMs) const;
    double personnesDepuisTable(double surface) const;
    void majRegime(const DensiteEstimation& base, qint64 maintenantMs);

    QVector<EchantillonToF> m_echantillons;
    qint64 m_dernierTMs = 0;
    double m_emaMm = -1.0;
    double m_hauteurPorteMm = 2100.0;
    double m_seuilPresenceMm = 1300.0;
    QVector<QPointF> m_table;
    bool m_tableChargee = false;
    QString m_regime = QStringLiteral("clustering");
    qint64 m_dernierBasculeMs = 0;
};
