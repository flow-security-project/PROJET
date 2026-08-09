#include "SalleDetailWidget.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QDateTime>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "data/DataSource.h"
#include "models/Alerte.h"
#include "models/AlerteModel.h"
#include "ui/IntegratedPlotWidget.h"
#include "ui/JaugeSaturation.h"

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
                                     AlerteModel* modele, QWidget* parent)
    : QWidget(parent)
    , m_source(source)
    , m_salleId(salleId)
    , m_modeleAlertes(modele)
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

    m_alerteFlux = new QLabel(QStringLiteral("FLUX SORTIE ANORMAL — sortie brusque détectée"), this);
    m_alerteFlux->setObjectName(QStringLiteral("alerteFluxBadge"));
    m_alerteFlux->setVisible(false);

    // --- Historique alertes de la salle (Prototype §3) ---
    auto* alerteBox = new QGroupBox(QStringLiteral("HISTORIQUE ALERTES SALLE"), this);
    alerteBox->setObjectName(QStringLiteral("alerteHistoriqueCard"));
    m_alerteListe = new QListWidget(alerteBox);
    m_alerteListe->setObjectName(QStringLiteral("alerteHistoriqueListe"));
    m_alerteListe->setWordWrap(true);
    m_alerteListe->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_alerteListe->setMaximumHeight(120);
    auto* alerteLayout = new QVBoxLayout(alerteBox);
    alerteLayout->setContentsMargins(12, 10, 12, 12);
    alerteLayout->addWidget(m_alerteListe);
    if (m_modeleAlertes) {
        connect(m_modeleAlertes, &AlerteModel::alerteAjoutee,
                this, [this](const Alerte&) { actualiserAlertes(); });
        connect(m_modeleAlertes, &AlerteModel::alerteModifiee,
                this, [this](const Alerte&) { actualiserAlertes(); });
    }

    // --- Zone anticipation de saturation ---
    auto* anticipationBox = new QGroupBox(QStringLiteral("ANTICIPATION DE SATURATION"), this);
    anticipationBox->setObjectName(QStringLiteral("anticipationCard"));

    m_anticipation = new QLabel(anticipationBox);
    m_anticipation->setObjectName(QStringLiteral("anticipationValue"));

    m_tendance = new QLabel(anticipationBox);
    m_tendance->setObjectName(QStringLiteral("anticipationSub"));

    m_jauge = new JaugeSaturation(anticipationBox);

    m_regime = new QLabel(anticipationBox);
    m_regime->setObjectName(QStringLiteral("anticipationSub"));
    m_confiance = new QLabel(anticipationBox);
    m_confiance->setObjectName(QStringLiteral("anticipationSub"));

    auto* metaRow = new QHBoxLayout;
    metaRow->setContentsMargins(0, 0, 0, 0);
    metaRow->setSpacing(16);
    metaRow->addWidget(m_regime);
    metaRow->addWidget(m_confiance);
    metaRow->addStretch();

    auto* anticipationLayout = new QVBoxLayout(anticipationBox);
    anticipationLayout->setContentsMargins(12, 10, 12, 12);
    anticipationLayout->setSpacing(4);
    anticipationLayout->addWidget(m_anticipation);
    anticipationLayout->addWidget(m_tendance);
    anticipationLayout->addWidget(m_jauge);
    anticipationLayout->addLayout(metaRow);

    auto* controls = new QHBoxLayout;
    auto* occupation = new QCheckBox(QStringLiteral("Occupation (#4A90D9)"), this);
    auto* entrees = new QCheckBox(QStringLiteral("Entrées (#2E7D32)"), this);
    auto* sorties = new QCheckBox(QStringLiteral("Sorties (#F57C00)"), this);
    auto* densite = new QCheckBox(QStringLiteral("Densité (#7C3AED)"), this);
    occupation->setObjectName(QStringLiteral("checkOccupation"));
    entrees->setObjectName(QStringLiteral("checkEntrees"));
    sorties->setObjectName(QStringLiteral("checkSorties"));
    densite->setObjectName(QStringLiteral("checkDensite"));
    occupation->setChecked(true);
    entrees->setChecked(true);
    sorties->setChecked(true);
    densite->setChecked(true);
    controls->addWidget(occupation);
    controls->addWidget(entrees);
    controls->addWidget(sorties);
    controls->addWidget(densite);
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
    connect(densite, &QCheckBox::toggled, m_plot,
            [this](bool visible) { m_plot->setGraphVisible(3, visible); });
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
    layout->addWidget(m_alerteFlux);
    layout->addWidget(anticipationBox);
    layout->addLayout(controls);
    layout->addWidget(m_plot, 1);
    layout->addWidget(alerteBox);

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
                                                          : QStringLiteral("offline");
    m_statut->setText(salle.statutTexte());
    m_statut->setProperty("level", level);
    m_statut->style()->unpolish(m_statut);
    m_statut->style()->polish(m_statut);
    m_alerteFlux->setVisible(salle.fluxSortieAnormal);
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

    const bool horsLigne = !salle.enLigne && !salle.enAttente;
    if (horsLigne || salle.occupation < 0) {
        m_anticipation->setText(QStringLiteral("—"));
        m_tendance->setText(QStringLiteral("Tendance : —"));
        m_regime->setText(QStringLiteral("Régime : —"));
        m_confiance->setText(QStringLiteral("Confiance : —"));
        m_jauge->setValeurs(0.0, -1, 0.0);
    } else {
        const int minAvant = salle.anticipationMin;
        QString niveau;
        if (minAvant < 0) {
            m_anticipation->setText(QStringLiteral("Aucune saturation prévue"));
            m_anticipation->setProperty("niveau", QStringLiteral("aucune"));
        } else if (minAvant == 0) {
            m_anticipation->setText(QStringLiteral("SALLE SATURÉE"));
            m_anticipation->setProperty("niveau", QStringLiteral("urgent"));
        } else if (minAvant <= 10) {
            m_anticipation->setText(QStringLiteral("Saturation prévue dans %1 min")
                                        .arg(minAvant));
            m_anticipation->setProperty("niveau", QStringLiteral("urgent"));
        } else if (minAvant <= 30) {
            m_anticipation->setText(QStringLiteral("Saturation prévue dans %1 min")
                                        .arg(minAvant));
            m_anticipation->setProperty("niveau", QStringLiteral("attention"));
        } else {
            m_anticipation->setText(QStringLiteral("Saturation prévue dans %1 min")
                                        .arg(minAvant));
            m_anticipation->setProperty("niveau", QStringLiteral("calme"));
        }
        m_anticipation->style()->unpolish(m_anticipation);
        m_anticipation->style()->polish(m_anticipation);

        m_tendance->setText(QStringLiteral("Tendance : %1 pers/min")
                                .arg(salle.penteTendance, 0, 'f', 1));
        m_regime->setText(QStringLiteral("Régime : %1").arg(salle.regime));
        m_confiance->setText(salle.confiance >= 0.0
                                 ? QStringLiteral("Confiance : %1 %")
                                       .arg(int(salle.confiance * 100.0))
                                 : QStringLiteral("Confiance : —"));
        m_jauge->setValeurs(salle.taux(), minAvant, salle.penteTendance);
    }

    m_plot->setSeries(salle.occHist, salle.densHist, salle.entHist, salle.sortHist,
                      salle.capacite);
    m_plot->setPrevision(salle.penteTendance, salle.anticipationMin,
                         salle.capacite);
    actualiserAlertes();
}

void SalleDetailWidget::actualiserAlertes()
{
    if (!m_alerteListe || !m_modeleAlertes)
        return;
    m_alerteListe->clear();
    const QList<Alerte> alertes = m_modeleAlertes->alertesPourSalle(m_salleId);
    for (const Alerte& a : alertes) {
        const QString heure
            = QDateTime::fromMSecsSinceEpoch(a.ts).toString(QStringLiteral("hh:mm:ss"));
        m_alerteListe->addItem(QStringLiteral("%1  %2  —  %3%4")
                                   .arg(heure, a.typeLibelle(), a.detail,
                                        a.acquittee ? QStringLiteral("  [ACQUITTÉE]")
                                                    : QString()));
    }
}
