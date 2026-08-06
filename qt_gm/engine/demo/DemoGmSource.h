#pragma once

#include <QTimer>

#include "engine/GmSource.h"

class DemoGmSource : public GmSource
{
    Q_OBJECT

public:
    explicit DemoGmSource(QObject* parent = nullptr);

    void start() override;
    void stop() override;
    QString type() const override { return "demo"; }

private slots:
    void onTick();

private:
    void majAmphi(int tick);
    void majB204(int tick);
    void majA102(int tick);

    QTimer m_timer;
    int m_tick = 0;
    bool m_amphiConfig = false;
    bool m_b204Config = false;
    bool m_a102Config = false;
};
