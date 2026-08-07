#include "SalleDetailWidget.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "data/DataSource.h"
#include "ui/IntegratedPlotWidget.h"

namespace {
QLabel* kpiLabel(const QString& title, QWidget* parent)
{
    auto* label = new QLabel(title, parent);
    label->setObjectName(QStringLiteral("kpiTitle"));
    return label;
}

QGroupBox* kpiBox(QWidget* parent)
{
    auto* box = new QGroupBox(parent);
    box->setObjectName(QStringLiteral("kpiCard"));
    return box;
}
}

SalleDetailWidget::SalleDetailWidget(DataSource* source, const QString& salleId,
                                     QWidget* parent)
    : QWidget(parent)
    , m_source(source)
    , m_salleId(salleId)
{
    setObjectName(QStringLiteral("detailRoot"));
    setAttribute(Qt::WA_StyledBackground, true);
    m_titre = new QLabel(this);
    m_titre->setObjectName(QStringLiteral("detailTitle"));
    m_statut = new QLabel(this);
    m_statut->setObjectName(QStringLiteral("detailStatusBadge"));

    auto* heading = new QHBoxLayout;
    heading->addWidget(m_titre);
    heading->addStretch();
    heading->addWidget(m_statut);

    auto* kpis = new QGridLayout;
    kpis->setSpacing(8);

    auto makeKpi = [this, kpis](const QString& title, const QString& kpiType, QLabel*& value,
                                int row, int column) {
        auto* box = kpiBox(this);
        auto* layout = new QVBoxLayout(box);
        layout->setContentsMargins(12, 10, 12, 10);
        layout->addWidget(kpiLabel(title, box));
        value = new QLabel(QStringLiteral("—"), box);
        value->setObjectName(QStringLiteral("kpiValue"));
        value->setProperty("kpiType", kpiType);
        layout->addWidget(value);
        kpis->addWidget(box, row, column);
    };
    makeKpi(QStringLiteral("OCCUPATION"), QStringLiteral("occupation"), m_occupation, 0, 0);
    makeKpi(QStringLiteral("TAUX"), QStringLiteral("taux"), m_taux, 0, 1);
    makeKpi(QStringLiteral("DÉBIT"), QStringLiteral("debit"), m_debit, 0, 2);
    makeKpi(QStringLiteral("ENTRÉES"), QStringLiteral("entrees"), m_entrees, 0, 3);
    makeKpi(QStringLiteral("SORTIES"), QStringLiteral("sorties"), m_sorties, 0, 4);

    m_infos = new QLabel(this);
    m_infos->setObjectName(QStringLiteral("detailInfos"));

    auto* controls = new QHBoxLayout;
    auto* occupation = new QCheckBox(QStringLiteral("Occupation (#4A90D9)"), this);
    auto* entrees = new QCheckBox(QStringLiteral("Entrées (#2E7D32)"), this);
    auto* sorties = new QCheckBox(QStringLiteral("Sorties (#F57C00)"), this);
    occupation->setObjectName(QStringLiteral("checkOccupation"));
    entrees->setObjectName(QStringLiteral("checkEntrees"));
    sorties->setObjectName(QStringLiteral("checkSorties"));
    occupation->setChecked(true);
    entrees->setChecked(true);
    sorties->setChecked(true);
    controls->addWidget(occupation);
    controls->addWidget(entrees);
    controls->addWidget(sorties);
    controls->addStretch();
    auto* pause = new QPushButton(QStringLiteral("Pause direct"), this);
    controls->addWidget(pause);

    m_plot = new IntegratedPlotWidget(this);
    m_plot->setObjectName(QStringLiteral("plotCard"));
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
    const QString level = salle.enAttente ? QStringLiteral("pending")
                                         : salle.enLigne ? QStringLiteral("normal")
                                                         : QStringLiteral("critical");
    m_statut->setText(salle.statutTexte());
    m_statut->setProperty("level", level);
    m_statut->style()->unpolish(m_statut);
    m_statut->style()->polish(m_statut);
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
