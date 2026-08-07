#pragma once

#include <QLabel>
#include <QWidget>

class StatusBar : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = nullptr);

    void setEvacuationGlobale(bool active);
    void setStatutMqtt(bool connecte, const QString& detail);
    void setStatutAsterisk(const QString& detail);
    void setNoeuds(int enLigne, int total);

private:
    QLabel* m_evac = nullptr;
    QLabel* m_mqtt = nullptr;
    QLabel* m_asterisk = nullptr;
    QLabel* m_noeuds = nullptr;
    bool m_evacActive = false;
    bool m_evacPulse = false;
};
