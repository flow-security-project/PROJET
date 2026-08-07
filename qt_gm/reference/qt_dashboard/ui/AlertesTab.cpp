#include "AlertesTab.h"

#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>

#include "widgets/Couleurs.h"

AlertesTab::AlertesTab(QWidget* parent)
    : QWidget(parent)
{
    m_filtre = new QComboBox(this);
    m_filtre->addItems({"Toutes", "Evacuation", "Bousculade", "Saturation",
                        "Immobile", "Intrusion", "Flux de sortie",
                        "Non acquittées"});

    auto* btnAcquitter = new QPushButton("Acquitter la sélection", this);
    auto* btnExport = new QPushButton("Exporter CSV", this);
    for (QPushButton* b : {btnAcquitter, btnExport}) {
        b->setStyleSheet(
            "QPushButton{background:#FFFFFF;border:1px solid #D0D0D0;border-radius:2px;"
            "padding:6px 10px;font-size:11px;} "
            "QPushButton:hover{border-color:#4A90D9;}");
    }

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels(
        {"Heure", "Salle", "Type", "Score", "Statut appel", "Acquittée"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);

    auto* layHaut = new QHBoxLayout;
    layHaut->addWidget(new QLabel("Filtre :", this));
    layHaut->addWidget(m_filtre);
    layHaut->addStretch();
    layHaut->addWidget(btnAcquitter);
    layHaut->addWidget(btnExport);

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(6);
    lay->addLayout(layHaut);
    lay->addWidget(m_table, 1);

    connect(btnAcquitter, &QPushButton::clicked, this, [this]() {
        const int r = m_table->currentRow();
        if (r < 0)
            return;
        const quint64 ts = m_table->item(r, 0)->data(Qt::UserRole).toULongLong();
        emit alerteAcquittee(m_table->item(r, 1)->data(Qt::UserRole).toString(), ts);
    });
    connect(btnExport, &QPushButton::clicked, this, [this]() {
        const QString chemin =
            QFileDialog::getSaveFileName(this, "Exporter les alertes",
                                         "alertes.csv", "CSV (*.csv)");
        if (chemin.isEmpty())
            return;
        QFile f(chemin);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
            return;
        QTextStream out(&f);
        out << "heure;salle;type;score;statut_appel;acquittee\n";
        for (int r = 0; r < m_table->rowCount(); r++) {
            for (int c = 0; c < 6; c++) {
                out << m_table->item(r, c)->text();
                if (c < 5)
                    out << ";";
            }
            out << "\n";
        }
    });
    connect(m_table, &QTableWidget::cellDoubleClicked, this,
            [this](int r, int) {
                const quint64 ts =
                    m_table->item(r, 0)->data(Qt::UserRole).toULongLong();
                emit alerteSelectionnee(ts);
            });
}

void AlertesTab::ajouterAlerte(const Alerte& a)
{
    const int r = m_table->rowCount();
    m_table->insertRow(r);
    auto* itHeure = new QTableWidgetItem(
        QDateTime::fromMSecsSinceEpoch(a.ts).toString("hh:mm:ss"));
    itHeure->setData(Qt::UserRole, a.ts);
    auto* itSalle = new QTableWidgetItem(a.salleNom);
    itSalle->setData(Qt::UserRole, a.salleId);
    auto* itType = new QTableWidgetItem(a.typeLibelle());
    itType->setBackground(QColor(severiteHex(a.severite())));
    itType->setForeground(Qt::white);
    auto* itScore = new QTableWidgetItem(QString::number(a.score));
    auto* itAppel = new QTableWidgetItem(a.appelStatutTexte());
    auto* itAcq = new QTableWidgetItem(a.acquittee ? "Oui" : "Non");
    m_table->setItem(r, 0, itHeure);
    m_table->setItem(r, 1, itSalle);
    m_table->setItem(r, 2, itType);
    m_table->setItem(r, 3, itScore);
    m_table->setItem(r, 4, itAppel);
    m_table->setItem(r, 5, itAcq);
    m_rangeeParTs.insert(a.ts, r);
}

void AlertesTab::modifierAlerte(const Alerte& a)
{
    const int r = m_rangeeParTs.value(a.ts, -1);
    if (r < 0)
        return;
    m_table->item(r, 4)->setText(a.appelStatutTexte());
    m_table->item(r, 5)->setText(a.acquittee ? "Oui" : "Non");
}

void AlertesTab::vider()
{
    m_table->setRowCount(0);
    m_rangeeParTs.clear();
}
