#include "SalleWindow.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "engine/GmSource.h"
#include "ui/PlotGm.h"

namespace {
QLabel* creerTitreKpi(const QString& texte, QWidget* parent)
{
    auto* l = new QLabel(texte, parent);
    l->setStyleSheet("font-size:10px;font-weight:700;color:#777777;letter-spacing:0.5px;");
    return l;
}

QFrame* creerBlocKpi(QWidget* parent)
{
    auto* f = new QFrame(parent);
    f->setStyleSheet(
        "QFrame{background:#FFFFFF;border:1px solid #D8D8D8;border-radius:4px;}"
        "QFrame:hover{border-color:#4A90D9;}");
    return f;
}
}

SalleWindow::SalleWindow(GmSource* source, const QString& salleId, QWidget* parent)
    : QWidget(parent)
    , m_source(source)
    , m_salleId(salleId)
{
    setWindowTitle("Salle " + salleId);
    resize(880, 620);

    // ===== En-tête : titre + statut =====
    m_titre = new QLabel(this);
    m_titre->setStyleSheet("font-size:18px;font-weight:700;color:#222222;");

    m_dot = new QLabel(this);
    m_dot->setFixedSize(10, 10);

    m_statutTexte = new QLabel(this);
    m_statutTexte->setStyleSheet("font-size:12px;font-weight:600;");

    auto* layTitre = new QHBoxLayout;
    layTitre->setSpacing(8);
    layTitre->addWidget(m_titre);
    layTitre->addWidget(m_dot);
    layTitre->addWidget(m_statutTexte);
    layTitre->addStretch();

    // ===== Box 1 : Occupation =====
    auto* kpiOcc = creerBlocKpi(this);
    auto* layOcc = new QVBoxLayout(kpiOcc);
    layOcc->setContentsMargins(12, 10, 12, 10);
    layOcc->setSpacing(4);
    layOcc->addWidget(creerTitreKpi("OCCUPATION EN DIRECT", this));

    auto* layOccVal = new QHBoxLayout;
    layOccVal->setSpacing(4);
    layOccVal->setContentsMargins(0, 0, 0, 0);
    m_occValeur = new QLabel("—", this);
    m_occValeur->setStyleSheet("font-size:20px;font-weight:700;color:#222222;");
    m_occCapacite = new QLabel("", this);
    m_occCapacite->setStyleSheet("font-size:12px;color:#777777;");
    layOccVal->addWidget(m_occValeur);
    layOccVal->addWidget(m_occCapacite);
    layOccVal->addStretch();

    m_occPct = new QLabel("—", this);
    m_occPct->setStyleSheet("font-size:11px;font-weight:700;");

    layOcc->addLayout(layOccVal);
    layOcc->addWidget(m_occPct);

    // ===== Box 2 : Débit Instantané =====
    auto* kpiDebit = creerBlocKpi(this);
    auto* layDebit = new QVBoxLayout(kpiDebit);
    layDebit->setContentsMargins(12, 10, 12, 10);
    layDebit->setSpacing(4);
    layDebit->addWidget(creerTitreKpi("DÉBIT INSTANTANÉ", this));

    m_debitValeur = new QLabel("0 pers/min", this);
    m_debitValeur->setStyleSheet("font-size:18px;font-weight:700;color:#1976D2;");
    m_debitTendance = new QLabel("Flux net calcule", this);
    m_debitTendance->setStyleSheet("font-size:11px;color:#777777;");

    layDebit->addWidget(m_debitValeur);
    layDebit->addWidget(m_debitTendance);

    // ===== Box 3 : Entrées Cumulées =====
    auto* kpiEnt = creerBlocKpi(this);
    auto* layEnt = new QVBoxLayout(kpiEnt);
    layEnt->setContentsMargins(12, 10, 12, 10);
    layEnt->setSpacing(4);
    layEnt->addWidget(creerTitreKpi("ENTRÉES CUMULÉES", this));

    m_entValeur = new QLabel("0", this);
    m_entValeur->setStyleSheet("font-size:20px;font-weight:700;color:#2E7D32;");
    auto* lblEntSous = new QLabel("Total enregistre", this);
    lblEntSous->setStyleSheet("font-size:11px;color:#777777;");

    layEnt->addWidget(m_entValeur);
    layEnt->addWidget(lblEntSous);

    // ===== Box 4 : Sorties Cumulées =====
    auto* kpiSort = creerBlocKpi(this);
    auto* laySort = new QVBoxLayout(kpiSort);
    laySort->setContentsMargins(12, 10, 12, 10);
    laySort->setSpacing(4);
    laySort->addWidget(creerTitreKpi("SORTIES CUMULÉES", this));

    m_sortValeur = new QLabel("0", this);
    m_sortValeur->setStyleSheet("font-size:20px;font-weight:700;color:#F57C00;");
    auto* lblSortSous = new QLabel("Total enregistre", this);
    lblSortSous->setStyleSheet("font-size:11px;color:#777777;");

    laySort->addWidget(m_sortValeur);
    laySort->addWidget(lblSortSous);

    // ===== Box 5 : Infos Salle =====
    auto* kpiInfo = creerBlocKpi(this);
    auto* layInfo = new QVBoxLayout(kpiInfo);
    layInfo->setContentsMargins(12, 10, 12, 10);
    layInfo->setSpacing(2);
    layInfo->addWidget(creerTitreKpi("INFORMATIONS SALLE", this));

    m_infoCapacite = new QLabel("Capacite: —", this);
    m_infoCapacite->setStyleSheet("font-size:11px;color:#333333;");
    m_infoHoraires = new QLabel("Horaires: —", this);
    m_infoHoraires->setStyleSheet("font-size:11px;color:#333333;");
    m_infoDensite = new QLabel("Densite: —", this);
    m_infoDensite->setStyleSheet("font-size:11px;color:#333333;");

    layInfo->addWidget(m_infoCapacite);
    layInfo->addWidget(m_infoHoraires);
    layInfo->addWidget(m_infoDensite);

    // ===== Organisation des 5 Box dans la carte entête =====
    auto* layKpis = new QHBoxLayout;
    layKpis->setSpacing(10);
    layKpis->addWidget(kpiOcc, 1);
    layKpis->addWidget(kpiDebit, 1);
    layKpis->addWidget(kpiEnt, 1);
    layKpis->addWidget(kpiSort, 1);
    layKpis->addWidget(kpiInfo, 1);

    auto* carteHeader = new QFrame(this);
    carteHeader->setStyleSheet("QFrame{background:#F8F9FA;border:1px solid #E0E0E0;border-radius:4px;}");
    auto* layHeader = new QVBoxLayout(carteHeader);
    layHeader->setContentsMargins(14, 12, 14, 12);
    layHeader->setSpacing(10);
    layHeader->addLayout(layTitre);
    layHeader->addLayout(layKpis);

    // ===== Carte graphique & barre de contrôles =====
    auto* carteGraph = new QFrame(this);
    carteGraph->setStyleSheet("QFrame{background:#FFFFFF;border:1px solid #E0E0E0;border-radius:4px;}");
    auto* layGraph = new QVBoxLayout(carteGraph);
    layGraph->setContentsMargins(14, 12, 14, 12);
    layGraph->setSpacing(8);

    // Barre d'outils du graphique (Légendes dynamiques + Cases à cocher + Pause)
    auto* layControles = new QHBoxLayout;
    layControles->setSpacing(12);

    auto* lblFiltres = new QLabel("COURBES VISIBLES :", this);
    lblFiltres->setStyleSheet("font-size:10px;font-weight:700;color:#777777;");
    layControles->addWidget(lblFiltres);

    m_chkOcc = new QCheckBox("Occupation (#4A90D9)", this);
    m_chkOcc->setChecked(true);
    m_chkOcc->setStyleSheet("QCheckBox{font-size:11px;font-weight:600;color:#4A90D9;}");

    m_chkEnt = new QCheckBox("Entrees (#2E7D32)", this);
    m_chkEnt->setChecked(true);
    m_chkEnt->setStyleSheet("QCheckBox{font-size:11px;font-weight:600;color:#2E7D32;}");

    m_chkSort = new QCheckBox("Sorties (#F57C00)", this);
    m_chkSort->setChecked(true);
    m_chkSort->setStyleSheet("QCheckBox{font-size:11px;font-weight:600;color:#F57C00;}");

    layControles->addWidget(m_chkOcc);
    layControles->addWidget(m_chkEnt);
    layControles->addWidget(m_chkSort);
    layControles->addStretch();

    m_btnPause = new QPushButton("Pause Direct", this);
    m_btnPause->setFixedWidth(100);
    m_btnPause->setStyleSheet(
        "QPushButton{background:#FFFFFF;border:1px solid #CCCCCC;border-radius:3px;padding:4px;font-size:11px;font-weight:600;}"
        "QPushButton:hover{background:#F0F0F0;border-color:#999999;}");
    layControles->addWidget(m_btnPause);

    m_plot = new PlotGm(this);

    layGraph->addLayout(layControles);
    layGraph->addWidget(m_plot, 1);

    auto* layMain = new QVBoxLayout(this);
    layMain->setContentsMargins(10, 10, 10, 10);
    layMain->setSpacing(10);
    layMain->addWidget(carteHeader);
    layMain->addWidget(carteGraph, 1);

    // Connexions
    connect(m_chkOcc, &QCheckBox::toggled, this, [this](bool c) { m_plot->setGraphVisible(0, c); });
    connect(m_chkEnt, &QCheckBox::toggled, this, [this](bool c) { m_plot->setGraphVisible(1, c); });
    connect(m_chkSort, &QCheckBox::toggled, this, [this](bool c) { m_plot->setGraphVisible(2, c); });

    connect(m_btnPause, &QPushButton::clicked, this, [this]() {
        const bool nouvState = !m_plot->isPaused();
        m_plot->setPause(nouvState);
        m_btnPause->setText(nouvState ? "Reprendre" : "Pause Direct");
    });

    connect(m_source, &GmSource::salleMiseAJour, this, &SalleWindow::onSalleMiseAJour);

    m_derniere = m_source->salles().value(m_salleId);
    afficher();
}

void SalleWindow::onSalleMiseAJour(const QString& id)
{
    if (id != m_salleId)
        return;
    m_derniere = m_source->salles().value(m_salleId);
    afficher();
}

double SalleWindow::calculerDebitInstantane() const
{
    const QVector<double>& hist = m_derniere.occHist;
    if (hist.size() < 2)
        return 0.0;
    const int pts = qMin(10, hist.size());
    const double delta = hist.last() - hist.at(hist.size() - pts);
    // 1 point par seconde -> debit par minute = delta / pts * 60
    return (delta / pts) * 60.0;
}

void SalleWindow::afficher()
{
    const SalleGm& s = m_derniere;

    m_titre->setText(s.nom.isEmpty() ? s.id : s.nom);
    setWindowTitle(QString("Salle %1").arg(s.nom.isEmpty() ? s.id : s.nom));

    if (s.enLigne) {
        m_dot->setStyleSheet("background:#2E7D32;border-radius:5px;");
        m_statutTexte->setText("CONNECTÉE");
        m_statutTexte->setStyleSheet("color:#2E7D32;");
    } else {
        m_dot->setStyleSheet("background:#C62828;border-radius:5px;");
        m_statutTexte->setText("HORS LIGNE");
        m_statutTexte->setStyleSheet("color:#C62828;");
    }

    // Box 1 : Occupation
    m_occValeur->setText(QString::number(s.occupation));
    m_occCapacite->setText(QString("/ %1").arg(s.capacite));

    const int pct = int(s.taux() * 100);
    QString statusTxt;
    QString coul;
    if (pct >= 90) {
        coul = "#C62828";
        statusTxt = QString("%1 %% - CRITIQUE").arg(pct);
    } else if (pct >= 70) {
        coul = "#F57C00";
        statusTxt = QString("%1 %% - AVERTISSEMENT").arg(pct);
    } else {
        coul = "#2E7D32";
        statusTxt = QString("%1 %% - NORMAL").arg(pct);
    }
    m_occPct->setText(statusTxt);
    m_occPct->setStyleSheet(QString("font-size:11px;font-weight:700;color:%1;").arg(coul));

    // Box 2 : Débit Instantané
    const double debit = calculerDebitInstantane();
    const QString signe = debit > 0 ? "+" : "";
    m_debitValeur->setText(QString("%1%2 pers/min").arg(signe).arg(debit, 0, 'f', 1));
    if (debit > 0.5) {
        m_debitTendance->setText("Tendance : En hausse");
        m_debitValeur->setStyleSheet("font-size:18px;font-weight:700;color:#C62828;");
    } else if (debit < -0.5) {
        m_debitTendance->setText("Tendance : En baisse");
        m_debitValeur->setStyleSheet("font-size:18px;font-weight:700;color:#2E7D32;");
    } else {
        m_debitTendance->setText("Tendance : Stable");
        m_debitValeur->setStyleSheet("font-size:18px;font-weight:700;color:#1976D2;");
    }

    // Box 3 & 4 : Entrées et Sorties
    m_entValeur->setText(QString::number(s.nbEntrees));
    m_sortValeur->setText(QString::number(s.nbSorties));

    // Box 5 : Infos Salle
    m_infoCapacite->setText(QString("Capacite: %1 pers").arg(s.capacite));
    m_infoHoraires->setText(QString("Horaires: %1 - %2")
                                .arg(s.horaireDebut.isEmpty() ? "--" : s.horaireDebut)
                                .arg(s.horaireFin.isEmpty() ? "--" : s.horaireFin));
    m_infoDensite->setText(QString("Densite: %1 p/m2").arg(s.densite, 0, 'f', 2));

    m_plot->setSeries(s.occHist, s.entHist, s.sortHist, s.capacite);
}

void SalleWindow::closeEvent(QCloseEvent* event)
{
    disconnect(m_source, &GmSource::salleMiseAJour,
               this, &SalleWindow::onSalleMiseAJour);
    QWidget::closeEvent(event);
}
