#include "SalleDetailWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QDateTime>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPrinter>
#include <QPushButton>
#include <QStyle>
#include <QTextDocument>
#include <QVBoxLayout>

#include "data/DataSource.h"
#include "history/HistoryManager.h"
#include "models/Alerte.h"
#include "models/AlerteModel.h"
#include "ui/AbSystemWidget.h"
#include "ui/IntegratedPlotWidget.h"
#include "ui/JaugeSaturation.h"
#include "ui/LedLcdWidget.h"

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
                                     AlerteModel* modele, HistoryManager* history,
                                     QWidget* parent)
    : QWidget(parent)
    , m_source(source)
    , m_salleId(salleId)
    , m_modeleAlertes(modele)
    , m_history(history)
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

    m_abSystem = new AbSystemWidget(this);

    m_ledLcd = new LedLcdWidget(this);

    m_alerteFlux = new QLabel(QStringLiteral("FLUX SORTIE ANORMAL — sortie brusque détectée"), this);
    m_alerteFlux->setObjectName(QStringLiteral("alerteFluxBadge"));
    m_alerteFlux->setVisible(false);

    m_alerteIntrusion = new QLabel(this);
    m_alerteIntrusion->setObjectName(QStringLiteral("alerteIntrusionBadge"));
    m_alerteIntrusion->setVisible(false);

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

    auto* controls = new QGridLayout;
    controls->setSpacing(8);
    auto* occupation = new QCheckBox(QStringLiteral("Occupation (#10B981)"), this);
    auto* entrees = new QCheckBox(QStringLiteral("Entrées (#10B981)"), this);
    auto* sorties = new QCheckBox(QStringLiteral("Sorties (#F57C00)"), this);
    auto* densite = new QCheckBox(QStringLiteral("Densité (#4ADE80)"), this);
    occupation->setObjectName(QStringLiteral("checkOccupation"));
    entrees->setObjectName(QStringLiteral("checkEntrees"));
    sorties->setObjectName(QStringLiteral("checkSorties"));
    densite->setObjectName(QStringLiteral("checkDensite"));
    occupation->setChecked(true);
    entrees->setChecked(true);
    sorties->setChecked(true);
    densite->setChecked(true);
    controls->addWidget(occupation, 0, 0);
    controls->addWidget(entrees, 0, 1);
    controls->addWidget(sorties, 1, 0);
    controls->addWidget(densite, 1, 1);
    controls->setColumnStretch(3, 1);
    auto* pause = new QPushButton(QStringLiteral("Pause direct"), this);
    controls->addWidget(pause, 0, 2, 2, 1, Qt::AlignVCenter);

    m_plot = new IntegratedPlotWidget(this);
    m_plot->setObjectName(QStringLiteral("plotCard"));
    m_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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

    auto* historiqueControls = new QHBoxLayout;
    historiqueControls->setContentsMargins(0, 4, 0, 0);
    historiqueControls->setSpacing(6);
    historiqueControls->addWidget(new QLabel(QStringLiteral("Historique :"), this));
    m_periode = new QComboBox(this);
    m_periode->addItem(QStringLiteral("Temps réel"));
    m_periode->addItem(QStringLiteral("Jour"));
    m_periode->addItem(QStringLiteral("Semaine"));
    m_periode->addItem(QStringLiteral("Mois"));
    m_periode->setObjectName(QStringLiteral("historiquePeriode"));
    historiqueControls->addWidget(m_periode);

    auto* charger = new QPushButton(QStringLiteral("Charger"), this);
    charger->setObjectName(QStringLiteral("btnChargerHistorique"));
    historiqueControls->addWidget(charger);
    auto* exporterCsvButton = new QPushButton(QStringLiteral("Exporter CSV"), this);
    exporterCsvButton->setObjectName(QStringLiteral("btnExporterSalleCsv"));
    historiqueControls->addWidget(exporterCsvButton);
    auto* exporterPdfButton = new QPushButton(QStringLiteral("Exporter PDF"), this);
    exporterPdfButton->setObjectName(QStringLiteral("btnExporterSallePdf"));
    historiqueControls->addWidget(exporterPdfButton);
    historiqueControls->addStretch();

    m_historiqueResume = new QLabel(this);
    m_historiqueResume->setObjectName(QStringLiteral("historiqueResume"));
    m_historiqueResume->setWordWrap(true);
    connect(m_periode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) { actualiserHistorique(); });
    connect(charger, &QPushButton::clicked,
            this, &SalleDetailWidget::actualiserHistorique);
    connect(exporterCsvButton, &QPushButton::clicked,
            this, &SalleDetailWidget::exporterCsv);
    connect(exporterPdfButton, &QPushButton::clicked,
            this, &SalleDetailWidget::exporterPdf);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    layout->addLayout(heading);
    layout->addLayout(kpis);

    // Corps en deux colonnes côte à côte : données/équipements à gauche,
    // courbe et contrôles à droite (la courbe reste visible en hauteur).
    auto* corps = new QHBoxLayout;
    corps->setSpacing(10);

    auto* colonneGaucheWidget = new QWidget(this);
    colonneGaucheWidget->setFixedWidth(680);
    auto* colonneGauche = new QVBoxLayout(colonneGaucheWidget);
    colonneGauche->setContentsMargins(0, 0, 0, 0);
    colonneGauche->setSpacing(8);
    colonneGauche->addWidget(m_infos);
    colonneGauche->addWidget(m_abSystem);
    colonneGauche->addWidget(m_ledLcd);
    colonneGauche->addWidget(m_alerteFlux);
    colonneGauche->addWidget(m_alerteIntrusion);
    colonneGauche->addWidget(anticipationBox);
    colonneGauche->addWidget(alerteBox);
    colonneGauche->addStretch();
    corps->addWidget(colonneGaucheWidget);

    auto* colonneDroite = new QVBoxLayout;
    colonneDroite->setSpacing(8);
    colonneDroite->addLayout(controls);
    colonneDroite->addLayout(historiqueControls);
    colonneDroite->addWidget(m_historiqueResume);
    colonneDroite->addWidget(m_plot, 1);
    corps->addLayout(colonneDroite, 1);

    layout->addLayout(corps, 1);

    connect(m_source, &DataSource::salleMiseAJour,
            this, &SalleDetailWidget::onSalleMiseAJour);
    connect(m_source, &DataSource::etatAB, this,
            [this](const QString& id, const QString& etat, qint64 tMs) {
                if (id == m_salleId)
                    m_abSystem->setEtat(etat, tMs);
            });
    connect(m_source, &DataSource::passageValide, this,
            [this](const QString& id, const QString& direction, qint64 tMs) {
                if (id == m_salleId)
                    m_abSystem->setDernierPassage(direction, tMs);
            });
    if (m_source && m_source->salles().contains(m_salleId))
        afficher(m_source->salles().value(m_salleId));
}

void SalleDetailWidget::onSalleMiseAJour(const QString& id)
{
    if (id != m_salleId || !m_source || !m_source->salles().contains(id))
        return;
    afficher(m_source->salles().value(id));
}

HistoryPeriod SalleDetailWidget::periodeSelectionnee() const
{
    if (!m_periode || m_periode->currentIndex() == 1)
        return HistoryPeriod::Day;
    if (m_periode->currentIndex() == 2)
        return HistoryPeriod::Week;
    return HistoryPeriod::Month;
}

void SalleDetailWidget::actualiserHistorique()
{
    if (!m_history || !m_historiqueResume || !m_periode)
        return;

    if (m_periode->currentIndex() == 0) {
        if (m_source && m_source->salles().contains(m_salleId)) {
            const Salle& salle = m_source->salles().value(m_salleId);
            m_plot->setSeries(salle.occHist, salle.densHist, salle.entHist,
                              salle.sortHist, salle.capacite, salle.timeHist);
            m_plot->setPrevision(salle.penteTendance, salle.anticipationMin,
                                 salle.capacite);
        }
        m_historiqueResume->setText(
            QStringLiteral("Vue temps réel : %1 points en mémoire glissante.")
                .arg(m_source && m_source->salles().contains(m_salleId)
                         ? m_source->salles().value(m_salleId).occHist.size()
                         : 0));
        return;
    }

    const HistoryPeriod period = periodeSelectionnee();
    const QVector<HistorySample> values = m_history->samples(m_salleId, period);
    const HistoryStats stats = m_history->analyse(m_salleId, period);
    const QList<Alerte> alertes = m_history->alertes(m_salleId, period);
    QVector<double> occupation;
    QVector<double> densite;
    QVector<double> entrees;
    QVector<double> sorties;
    occupation.reserve(values.size());
    densite.reserve(values.size());
    entrees.reserve(values.size());
    sorties.reserve(values.size());
    for (const HistorySample& value : values) {
        occupation.append(value.occupation);
        densite.append(value.densite);
        entrees.append(value.entrees);
        sorties.append(value.sorties);
    }
    const int capacite = m_source && m_source->salles().contains(m_salleId)
                             ? m_source->salles().value(m_salleId).capacite
                             : 0;
    m_plot->setHistoricalSeries(values, capacite);
    m_plot->setPrevision(0.0, -1, capacite);
    const QString pic = stats.aUnPic
                            ? QDateTime::fromMSecsSinceEpoch(stats.pic.timestampMs)
                                  .toString(QStringLiteral("dd/MM HH:mm"))
                            : QStringLiteral("—");
    const QString creux = stats.aUnCreux
                              ? QDateTime::fromMSecsSinceEpoch(stats.creux.timestampMs)
                                    .toString(QStringLiteral("dd/MM HH:mm"))
                              : QStringLiteral("—");
    m_historiqueResume->setText(
        QStringLiteral("%1 : %2 points | occupation moyenne : %3 pers. | densité moyenne : %4 | "
                       "entrées : %5 | sorties : %6 | alertes : %7 | pic : %8 (%9 pers.) | "
                       "creux : %10 (%11 pers.)")
            .arg(HistoryManager::libellePeriode(period))
            .arg(stats.nombrePoints)
            .arg(stats.occupationMoyenne, 0, 'f', 1)
            .arg(stats.densiteMoyenne, 0, 'f', 2)
            .arg(stats.nombreEntrees)
            .arg(stats.nombreSorties)
            .arg(alertes.size())
            .arg(pic)
            .arg(stats.aUnPic ? stats.pic.occupation : 0)
            .arg(creux)
            .arg(stats.aUnCreux ? stats.creux.occupation : 0));
}

void SalleDetailWidget::exporterCsv()
{
    if (!m_history || !m_periode)
        return;
    const HistoryPeriod period = periodeSelectionnee();
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Exporter l'historique de la salle"),
        QStringLiteral("%1_%2.csv")
            .arg(m_salleId, HistoryManager::libellePeriode(period).toLower()),
        QStringLiteral("Fichiers CSV (*.csv)"));
    if (path.isEmpty())
        return;

    QString error;
    if (!m_history->exportSalleCsv(m_salleId, period, path, &error)) {
        QMessageBox::warning(this, QStringLiteral("Export impossible"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Export terminé"),
                             QStringLiteral("Historique de la salle exporté."));
}

void SalleDetailWidget::exporterPdf()
{
    if (!m_history || !m_source || !m_source->salles().contains(m_salleId)
        || !m_periode) {
        return;
    }

    const HistoryPeriod period = periodeSelectionnee();
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Exporter le rapport de la salle"),
        QStringLiteral("%1_%2.pdf")
            .arg(m_salleId, HistoryManager::libellePeriode(period).toLower()),
        QStringLiteral("Fichiers PDF (*.pdf)"));
    if (path.isEmpty())
        return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    QTextDocument document;
    const Salle& salle = m_source->salles().value(m_salleId);
    const HistoryStats stats = m_history->analyse(m_salleId, period);
    const QVector<PassageEvent> passages = m_history->passages(m_salleId, period);
    const QList<Alerte> alertes = m_history->alertes(m_salleId, period);
    const QString title = salle.nom.isEmpty() ? salle.id
                                              : QStringLiteral("%1 (%2)")
                                                    .arg(salle.nom, salle.id);
    QString passagesHtml;
    for (const PassageEvent& passage : passages) {
        passagesHtml += QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td></tr>")
                            .arg(QDateTime::fromMSecsSinceEpoch(passage.timestampMs)
                                     .toString(QStringLiteral("dd/MM/yyyy HH:mm:ss")),
                                 passage.direction,
                                 QString::number(passage.occupation));
    }
    if (passagesHtml.isEmpty())
        passagesHtml = QStringLiteral("<tr><td colspan=\"3\">Aucun passage</td></tr>");

    QString alertesHtml;
    for (const Alerte& alerte : alertes) {
        alertesHtml += QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td></tr>")
                           .arg(QDateTime::fromMSecsSinceEpoch(qint64(alerte.ts))
                                    .toString(QStringLiteral("dd/MM/yyyy HH:mm:ss")),
                                alerte.typeLibelle(), alerte.detail.toHtmlEscaped());
    }
    if (alertesHtml.isEmpty())
        alertesHtml = QStringLiteral("<tr><td colspan=\"3\">Aucune alerte</td></tr>");
    document.setHtml(
        QStringLiteral("<h1>Rapport historique — %1</h1>"
                       "<p>Période : <b>%2</b><br/>Généré le : %3</p>"
                       "<h2>Statistiques</h2>"
                       "<ul><li>Points agrégés : %4</li>"
                       "<li>Occupation moyenne : %5 personnes</li>"
                       "<li>Densité moyenne : %6</li>"
                       "<li>Entrées enregistrées : %7</li>"
                       "<li>Sorties enregistrées : %8</li>"
                       "<li>Pic d'occupation : %9 personnes</li>"
                       "<li>Creux d'occupation : %10 personnes</li></ul>"
                       "<h2>État courant</h2>"
                       "<p>Occupation : %11<br/>Capacité : %12<br/>Densité : %13</p>"
                       "<h2>Passages (%14)</h2>"
                       "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">"
                       "<tr><th>Heure</th><th>Direction</th><th>Occupation</th></tr>%15</table>"
                       "<h2>Alertes (%16)</h2>"
                       "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">"
                       "<tr><th>Heure</th><th>Type</th><th>Détail</th></tr>%17</table>")
            .arg(title)
            .arg(HistoryManager::libellePeriode(period))
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
            .arg(stats.nombrePoints)
            .arg(stats.occupationMoyenne, 0, 'f', 1)
            .arg(stats.densiteMoyenne, 0, 'f', 2)
            .arg(stats.nombreEntrees)
            .arg(stats.nombreSorties)
            .arg(stats.aUnPic ? stats.pic.occupation : 0)
            .arg(stats.aUnCreux ? stats.creux.occupation : 0)
            .arg(salle.occupationTexte())
            .arg(salle.capacite)
            .arg(salle.densite, 0, 'f', 2)
            .arg(passages.size())
            .arg(passagesHtml)
            .arg(alertes.size())
            .arg(alertesHtml));
    document.print(&printer);
    QMessageBox::information(this, QStringLiteral("Export terminé"),
                             QStringLiteral("Rapport PDF de la salle exporté."));
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
                          : !salle.enLigne ? QStringLiteral("offline")
                          : salle.intrusionActive ? QStringLiteral("critical")
                                                  : QStringLiteral("normal");
    m_statut->setText(salle.statutTexte());
    m_statut->setProperty("level", level);
    m_statut->style()->unpolish(m_statut);
    m_statut->style()->polish(m_statut);
    m_alerteFlux->setVisible(salle.fluxSortieAnormal);
    if (salle.intrusionActive) {
        const int minutes = int(salle.intrusionDureeS) / 60;
        const int secondes = int(salle.intrusionDureeS) % 60;
        m_alerteIntrusion->setText(
            QStringLiteral("INTRUSION HORS HORAIRES — présence détectée en dehors des "
                           "horaires autorisés (%1-%2) depuis %3 min %4 s")
                .arg(salle.horaireDebut, salle.horaireFin)
                .arg(minutes)
                .arg(secondes));
    }
    m_alerteIntrusion->setVisible(salle.intrusionActive);
    m_occupation->setText(QStringLiteral("%1").arg(salle.occupationTexte()));
    m_taux->setText(QStringLiteral("%1 %").arg(int(salle.taux() * 100.0)));
    m_debit->setText(QStringLiteral("%1 pers/min").arg(debitInstantane(salle), 0, 'f', 1));
    m_entrees->setText(QString::number(salle.nbEntrees));
    m_sorties->setText(QString::number(salle.nbSorties));
    m_infos->setText(
        QStringLiteral("Capacité : %1 personnes   |   Densité : %2   |   Horaires : %3 - %4\n"
                       "Hauteur porte : %5   |   Personnes estimées : %6 (régime %7)")
            .arg(salle.capacite)
            .arg(salle.densite, 0, 'f', 2)
            .arg(salle.horaireDebut, salle.horaireFin)
            .arg(salle.hauteurPorteMesuree
                     ? QStringLiteral("%1 cm").arg(salle.hauteurPorteCm, 0, 'f', 1)
                     : QStringLiteral("non mesurée"))
            .arg(salle.nbPersonnesEstime >= 0
                     ? QString::number(salle.nbPersonnesEstime)
                     : QStringLiteral("—"))
            .arg(salle.regime));

    m_ledLcd->setAffichage(salle.ledCouleur, salle.ledCouleurConfirmee, salle.ledMode,
                           salle.lcdLigne1, salle.lcdLigne2, salle.enLigne, salle.enAttente);

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
                      salle.capacite, salle.timeHist);
    m_plot->setPrevision(salle.penteTendance, salle.anticipationMin,
                         salle.capacite);
    actualiserAlertes();
    actualiserHistorique();
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
