#include "AlertPanel.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

#include "widgets/Couleurs.h"

AlertPanel::AlertPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* titre = new QLabel("ALERTES", this);
    titre->setStyleSheet(
        "font-size:12px;font-weight:700;color:#1A1A1A;padding:4px 2px;");

    m_filtreNonAcq = new QCheckBox("Non acquittées uniquement", this);
    m_filtreNonAcq->setStyleSheet("font-size:10px;color:#555555;");

    auto* zone = new QScrollArea(this);
    zone->setWidgetResizable(true);
    zone->setStyleSheet("QScrollArea{border:none;background:transparent;}");
    m_liste = new QWidget(zone);
    m_layListe = new QVBoxLayout(m_liste);
    m_layListe->setContentsMargins(0, 0, 0, 0);
    m_layListe->setSpacing(6);
    m_layListe->addStretch();
    zone->setWidget(m_liste);

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->setSpacing(6);
    lay->addWidget(titre);
    lay->addWidget(m_filtreNonAcq);
    lay->addWidget(zone, 1);

    connect(m_filtreNonAcq, &QCheckBox::toggled, this, [this](bool) {
        m_layListe->removeItem(m_layListe->itemAt(m_layListe->count() - 1));
        while (QLayoutItem* item = m_layListe->takeAt(0)) {
            if (item->widget())
                item->widget()->deleteLater();
            delete item;
        }
        m_layListe->addStretch();
        for (auto it = m_alertes.begin(); it != m_alertes.end(); ++it) {
            if (!(m_filtreNonAcq->isChecked() && it->acquittee))
                ajouterRangee(it.value());
        }
    });
}

void AlertPanel::ajouterAlerte(const Alerte& a)
{
    m_alertes.insert(a.ts, a);
    if (m_filtreNonAcq->isChecked() && a.acquittee)
        return;
    ajouterRangee(a);
}

void AlertPanel::ajouterRangee(const Alerte& a)
{
    auto* rangee = new QFrame(m_liste);
    rangee->setObjectName("carteAlerte");
    rangee->setProperty("salleId", a.salleId);
    rangee->setProperty("ts", a.ts);
    rangee->setStyleSheet(
        "QFrame#carteAlerte{background:#F5F5F5;border:1px solid #D0D0D0;"
        "border-radius:2px;}");

    auto* badge = new QLabel(a.typeLibelle(), rangee);
    badge->setObjectName(severiteObjectName(a.severite()));
    badge->setStyleSheet(
        QString("background:%1;color:#FFFFFF;font-size:9px;font-weight:700;"
                "padding:2px 6px;border-radius:2px;")
            .arg(severiteHex(a.severite())));

    auto* detail = new QLabel(rangee);
    detail->setWordWrap(true);
    detail->setStyleSheet("font-size:10px;color:#555555;");
    detail->setText(
        QString("%1 · %2  —  %3\n%4\nAppel : %5")
            .arg(a.salleNom)
            .arg(QDateTime::fromMSecsSinceEpoch(a.ts).toString("hh:mm:ss"))
            .arg(a.detail)
            .arg(a.acquittee ? "ACQUITTÉE" : "non acquittée")
            .arg(a.appelStatutTexte()));

    auto* btnOk = new QPushButton("✓", rangee);
    btnOk->setToolTip("Acquitter");
    btnOk->setFixedSize(24, 24);
    btnOk->setStyleSheet(
        "QPushButton{background:#FFFFFF;border:1px solid #2E7D32;border-radius:2px;"
        "color:#2E7D32;font-weight:700;} QPushButton:hover{background:#2E7D32;color:#FFFFFF;}");
    auto* btnVoir = new QPushButton("Voir", rangee);
    btnVoir->setToolTip("Voir le détail dans la salle");
    btnVoir->setStyleSheet(
        "QPushButton{background:#FFFFFF;border:1px solid #D0D0D0;border-radius:2px;"
        "font-size:9px;padding:2px 6px;} QPushButton:hover{border-color:#4A90D9;}");

    auto* layBtns = new QVBoxLayout;
    layBtns->setSpacing(4);
    layBtns->addWidget(btnOk);
    layBtns->addWidget(btnVoir);
    layBtns->addStretch();

    auto* lay = new QHBoxLayout(rangee);
    lay->setContentsMargins(8, 6, 6, 6);
    lay->setSpacing(8);
    lay->addWidget(badge, 0, Qt::AlignTop);
    lay->addWidget(detail, 1);
    lay->addLayout(layBtns);

    m_layListe->insertWidget(m_layListe->count() - 1, rangee);

    connect(btnOk, &QPushButton::clicked, this, [this, rangee]() {
        emit alerteAcquittee(rangee->property("salleId").toString(),
                             rangee->property("ts").toULongLong());
    });
    connect(btnVoir, &QPushButton::clicked, this, [this, rangee]() {
        emit voirDetail(rangee->property("salleId").toString(),
                        rangee->property("ts").toULongLong());
    });
}

void AlertPanel::vider()
{
    m_alertes.clear();
    m_layListe->removeItem(m_layListe->itemAt(m_layListe->count() - 1));
    while (QLayoutItem* item = m_layListe->takeAt(0)) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }
    m_layListe->addStretch();
}

void AlertPanel::modifierAlerte(const Alerte& a)
{
    m_alertes.insert(a.ts, a);
    if (m_filtreNonAcq->isChecked() && a.acquittee) {
        for (int i = 0; i < m_layListe->count() - 1; i++) {
            QLayoutItem* item = m_layListe->itemAt(i);
            if (item->widget()
                && item->widget()->property("ts").toULongLong() == a.ts) {
                item->widget()->deleteLater();
                m_layListe->removeItem(item);
                delete item;
                return;
            }
        }
    }
}
