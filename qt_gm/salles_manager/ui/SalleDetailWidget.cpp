#include "SalleDetailWidget.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "data/DataSource.h"
#include "ui/IntegratedPlotWidget.h"

namespace {
QLabel* kpiLabel(const QString& title, QWidget* parent)
{
    auto* label = new QLabel(title, parent);
    label->setStyleSheet(QStringLiteral("font-size:10px;font-weight:700;color:#777777;"));
    return label;
}

QGroupBox* kpiBox(QWidget* parent)
{
    auto* box = new QGroupBox(parent);
    box->setStyleSheet(
        QStringLiteral("QGroupBox{background:#FFFFFF;border:1px solid #D8D8D8;"
                       "border-radius:2px;margin-top:8px;padding:8px;}"));
    return box;
}
}

SalleDetailWidget::SalleDetailWidget(DataSource* source, const QString& salleId,
                                     QWidget* parent)
    : QWidget(parent)
    , m_source(source)
    , m_salleId(salleId)
{
    m_titre = new QLabel(this);
    m_titre->setStyleSheet(QStringLiteral("font-size:18px;font-weight:700;color:#222222;"));
    m_statut = new QLabel(this);

    auto* heading = new QHBoxLayout;
    heading->addWidget(m_titre);
    heading->addStretch();
    heading->addWidget(m_statut);

    auto* kpis = new QGridLayout;
    kpis->setSpacing(8);

    auto makeKpi = [this, kpis](const QString& title, QLabel*& value,
                                int row, int column, const QString& color) {
        auto* box = kpiBox(this);
        auto* layout = new QVBoxLayout(box);
        layout->setContentsMargins(10, 8, 10, 8);
        layout->addWidget(kpiLabel(title, box));
        value = new QLabel(QStringLiteral("—"), box);
        value->setStyleSheet(QStringLiteral("font-size:20px;font-weight:700;color:%1;")
                                  .arg(color));
        layout->addWidget(value);
        kpis->addWidget(box, row, column);
    };
    makeKpi(QStringLiteral("OCCUPATION"), m_occupation, 0, 0, QStringLiteral("#222222"));
    makeKpi(QStringLiteral("TAUX"), m_taux, 0, 1, QStringLiteral("#1976D2"));
    makeKpi(QStringLiteral("DÉBIT"), m_debit, 0, 2, QStringLiteral("#1976D2"));
    makeKpi(QStringLiteral("ENTRÉES"), m_entrees, 0, 3, QStringLiteral("#2E7D32"));
    makeKpi(QStringLiteral("SORTIES"), m_sorties, 0, 4, QStringLiteral("#F57C00"));

    m_infos = new QLabel(this);
    m_infos->setStyleSheet(QStringLiteral("font-size:11px;color:#555555;"));

    auto* controls = new QHBoxLayout;
    auto* occupation = new QCheckBox(QStringLiteral("Occupation (#4A90D9)"), this);
    auto* entrees = new QCheckBox(QStringLiteral("Entrées (#2E7D32)"), this);
    auto* sorties = new QCheckBox(QStringLiteral("Sorties (#F57C00)"), this);
    occupation->setChecked(true);
    entrees->setChecked(true);
    sorties->setChecked(true);
    occupation->setStyleSheet(QStringLiteral("color:#4A90D9;font-weight:600;"));
    entrees->setStyleSheet(QStringLiteral("color:#2E7D32;font-weight:600;"));
    sorties->setStyleSheet(QStringLiteral("color:#F57C00;font-weight:600;"));
    controls->addWidget(occupation);
    controls->addWidget(entrees);
    controls->addWidget(sorties);
    controls->addStretch();
    auto* pause = new QPushButton(QStringLiteral("Pause direct"), this);
    controls->addWidget(pause);

    m_plot = new IntegratedPlotWidget(this);
    connect(occupation, &QCheckBox::toggled, m_plot,
            [this](bool visible) { m_plot->setGraphVisible(0, visible); });
    connect(entrees, &QCheckBox::toggled, m_plot,
            [this](bool visible) { m_plot->setGraphVisible(1, visible); });
    connect(sorties, &QCheckBox::toggled, m_plot,
            [this](bool visible) { m_plot->setGraphVisible(2, visible); });
    connect(pause, &QPushButton::clicked, this, [this, pause]() {
        const bool paused = !m_plot->isPaused();
        m_plot->setPause(paused);
        pause->setText(paused ? QStringLiteral("Reprendre")
                              : QStringLiteral("Pause direct"));
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    layout->addLayout(heading);
    layout->addLayout(kpis);
    layout->addWidget(m_infos);
    layout->addLayout(controls);
    layout->addWidget(m_plot, 1);

    connect(m_source, &DataSource::salleMiseAJour,
            this, &SalleDetailWidget::onSalleMiseAJour);
    if (m_source && m_source->salles().contains(m_salleId))
        afficher(m_source->salles().value(m_salleId));
}

void SalleDetailWidget::onSalleMiseAJour(const QString& id)
{
    if (id != m_salleId || !m_source || !m_source->salles().contains(id))
        return;
    afficher(m_source->salles().value(id));
}

double SalleDetailWidget::debitInstantane(const Salle& salle) const
{
    if (salle.occHist.size() < 2)
        return 0.0;
    const int points = qMin(10, salle.occHist.size());
    const int intervales = qMax(1, points - 1);
    return (salle.occHist.last() - salle.occHist.at(salle.occHist.size() - points))
           / intervales * 60.0;
}

void SalleDetailWidget::afficher(const Salle& salle)
{
    m_titre->setText(salle.nom.isEmpty() ? salle.id
                                        : QStringLiteral("%1 (%2)").arg(salle.nom, salle.id));
    const QString statusColor = salle.enAttente ? QStringLiteral("#F57C00")
                                                : salle.enLigne ? QStringLiteral("#2E7D32")
                                                                : QStringLiteral("#C62828");
    m_statut->setText(salle.statutTexte());
    m_statut->setStyleSheet(QStringLiteral("color:%1;font-weight:700;").arg(statusColor));
    m_occupation->setText(QStringLiteral("%1").arg(salle.occupationTexte()));
    m_taux->setText(QStringLiteral("%1 %").arg(int(salle.taux() * 100.0)));
    m_debit->setText(QStringLiteral("%1 pers/min").arg(debitInstantane(salle), 0, 'f', 1));
    m_entrees->setText(QString::number(salle.nbEntrees));
    m_sorties->setText(QString::number(salle.nbSorties));
    m_infos->setText(
        QStringLiteral("Capacité : %1 personnes   |   Densité : %2   |   Horaires : %3 - %4\n"
                       "Hauteur porte : %5")
            .arg(salle.capacite)
            .arg(salle.densite, 0, 'f', 2)
            .arg(salle.horaireDebut, salle.horaireFin)
            .arg(salle.hauteurPorteMesuree
                     ? QStringLiteral("%1 cm").arg(salle.hauteurPorteCm, 0, 'f', 1)
                     : QStringLiteral("non mesurée")));
    m_plot->setSeries(salle.occHist, salle.entHist, salle.sortHist, salle.capacite);
}
