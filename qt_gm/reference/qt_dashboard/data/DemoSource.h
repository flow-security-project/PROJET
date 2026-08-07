#pragma once

#include <QTimer>

#include "DataSource.h"

class DemoSource : public DataSource
{
    Q_OBJECT

public:
    explicit DemoSource(QObject* parent = nullptr);

    void start() override;
    void stop() override;
    QString type() const override { return "demo"; }

    void envoyerConfig(const Salle& s) override;
    void commanderTest(const QString& salleId, const QString& composant,
                       const QString& valeur) override;
    void forcerEvacuation(const QString& salleId, bool actif) override;
    void resetAlertesSalle(const QString& salleId) override;

private slots:
    void onTick();

private:
    void initSalles();
    void majAmphi();
    void majB204();
    void majTp3();
    void majCouloir();
    void majA102();
    void majCourbes(const QString& id);
    void armerAppel(quint64 ts, const QString& statutFinal, int dansTicks);

    struct AppelPending {
        quint64 ts = 0;
        QString statutFinal;
        int finTick = 0;
    };

    QTimer m_timer;
    int m_tick = 0;
    quint64 m_tsBase = 0;
    QVector<AppelPending> m_appelsPending;

    bool m_amphiSaturationAlertee = false;
    bool m_b204Bousculade = false;
    bool m_b204EvacFinie = false;
    bool m_tp3Intrusion = false;
    bool m_couloirImmobile = false;
    bool m_a102FluxAlerte = false;
    bool m_amphiEvacFinie = false;
};
