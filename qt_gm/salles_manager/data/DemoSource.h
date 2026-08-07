#pragma once

#include <QTimer>

#include "data/DataSource.h"

class DemoSource : public DataSource
{
    Q_OBJECT

public:
    explicit DemoSource(QObject* parent = nullptr);

    void start() override;
    void stop() override;
    QString type() const override { return QStringLiteral("demo"); }

    void creerSalle(const Salle& salle) override;
    void modifierSalle(const Salle& salle) override;
    void getHauteurPorte(const QString& salleId) override;
    void actualiserSalle(const QString& salleId) override;

private slots:
    void onTick();

private:
    QTimer m_timer;
    int m_tick = 0;
};
