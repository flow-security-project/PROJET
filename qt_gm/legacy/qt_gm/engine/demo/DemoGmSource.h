#pragma once

#include <QSet>
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
    void ajouterSalle(const SalleGm& salle);
    void activerSynchronisation(const QString& fichier);

private slots:
    void onTick();
    void onSynchronisationTick();

private:
    void majAmphi(int tick);
    void majB204(int tick);
    void majA102(int tick);

    QTimer m_timer;
    QTimer m_synchronisationTimer;
    int m_tick = 0;
    QSet<QString> m_sallesAjoutees;
    QString m_synchronisationFichier;
    QString m_synchronisationId;
    bool m_amphiConfig = false;
    bool m_b204Config = false;
    bool m_a102Config = false;
};
