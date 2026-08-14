#pragma once

#include <QHash>
#include <QTimer>

#include "data/DataSource.h"

class DensiteEstimator;
class IntrusionDetector;
class PassageDetectorAB;

class DemoSource : public DataSource
{
    Q_OBJECT

public:
    explicit DemoSource(QObject* parent = nullptr);
    ~DemoSource() override;

    void start() override;
    void stop() override;
    QString type() const override { return QStringLiteral("demo"); }

    void creerSalle(const Salle& salle) override;
    void modifierSalle(const Salle& salle) override;
    void supprimerSalle(const QString& id) override;
    void getHauteurPorte(const QString& salleId) override;
    void actualiserSalle(const QString& salleId) override;

private slots:
    void onTick();

private:
    void preparerPassageAB(const QString& salleId);
    void simulerEntree(const QString& salleId);
    void simulerSortie(const QString& salleId);
    void simulerTof(Salle& salle, qint64 maintenantMs);
    void verifierIntrusion(const QString& salleId, qint64 maintenantMs);
    void majDecisionsFlux();

    QTimer m_timer;
    int m_tick = 0;
    QHash<QString, double> m_fluxAccum;
    QHash<QString, DensiteEstimator*> m_densite;
    QHash<QString, IntrusionDetector*> m_intrusion;
    QHash<QString, PassageDetectorAB*> m_detecteursPassage;
    QHash<QString, qint64> m_dernierTofMs;
    QHash<QString, bool> m_presenceToF;
    QHash<QString, QString> m_dernieresCiblesFlux;
};
