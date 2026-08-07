#include "StatusBar.h"

#include <QHBoxLayout>
#include <QTimer>

StatusBar::StatusBar(QWidget* parent)
    : QWidget(parent)
{
    m_evac = new QLabel("ÉVACUATION : normale", this);
    m_evac->setObjectName("badgeAttention");
    m_mqtt = new QLabel("MQTT : —", this);
    m_mqtt->setObjectName("badgeOffline");
    m_asterisk = new QLabel("ASTERISK : —", this);
    m_asterisk->setObjectName("badgeOffline");
    m_noeuds = new QLabel("NŒUDS : 0/0", this);
    m_noeuds->setObjectName("badgeInfo");

    for (QLabel* l : {m_evac, m_mqtt, m_asterisk, m_noeuds})
        l->setStyleSheet(
            "font-size:11px;font-weight:600;padding:3px 10px;border-radius:10px;");

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 4);
    lay->setSpacing(8);
    lay->addWidget(m_evac);
    lay->addWidget(m_mqtt);
    lay->addWidget(m_asterisk);
    lay->addWidget(m_noeuds);
    lay->addStretch();

    auto* cligno = new QTimer(this);
    cligno->setInterval(500);
    connect(cligno, &QTimer::timeout, this, [this]() {
        if (!m_evacActive)
            return;
        m_evacPulse = !m_evacPulse;
        m_evac->setStyleSheet(
            QString("font-size:11px;font-weight:700;padding:3px 10px;"
                    "border-radius:10px;background:%1;color:#FFFFFF;")
                .arg(m_evacPulse ? "#C62828" : "#F57C00"));
    });
    cligno->start();
}

void StatusBar::setEvacuationGlobale(bool active)
{
    m_evacActive = active;
    if (active) {
        m_evacPulse = true;
        m_evac->setText("ÉVACUATION : EN COURS");
        m_evac->setStyleSheet(
            "font-size:11px;font-weight:700;padding:3px 10px;border-radius:10px;"
            "background:#C62828;color:#FFFFFF;");
    } else {
        m_evacPulse = false;
        m_evac->setText("ÉVACUATION : normale");
        m_evac->setStyleSheet(
            "font-size:11px;font-weight:600;padding:3px 10px;border-radius:10px;"
            "background:#2E7D32;color:#FFFFFF;");
    }
}

void StatusBar::setStatutMqtt(bool connecte, const QString& detail)
{
    m_mqtt->setText(QString("MQTT : %1").arg(detail.isEmpty() ? (connecte ? "OK" : "KO") : detail));
    m_mqtt->setStyleSheet(
        QString("font-size:11px;font-weight:600;padding:3px 10px;border-radius:10px;"
                "background:%1;color:#FFFFFF;")
            .arg(connecte ? "#2E7D32" : "#C62828"));
}

void StatusBar::setStatutAsterisk(const QString& detail)
{
    m_asterisk->setText("ASTERISK : " + detail);
    const bool ok = (detail == "Enregistré");
    m_asterisk->setStyleSheet(
        QString("font-size:11px;font-weight:600;padding:3px 10px;border-radius:10px;"
                "background:%1;color:#FFFFFF;")
            .arg(ok ? "#2E7D32" : "#C62828"));
}

void StatusBar::setNoeuds(int enLigne, int total)
{
    m_noeuds->setText(QString("NŒUDS : %1/%2").arg(enLigne).arg(total));
}
