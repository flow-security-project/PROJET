#include "MainWindow.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTime>
#include <QToolBar>
#include <QVBoxLayout>
#include <QVariant>

#include "data/Alerte.h"
#include "data/Salle.h"
#include "ui/AlertesTab.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("i++ v4.0 — Gestion des flux & évacuation sécurisée");
    resize(1440, 900);

    m_demo = new DemoSource(this);
    m_mqtt = new MqttSource(this);
    m_source = m_demo;

    // --- Barre de source -------------------------------------------------
    m_sourceBox = new QComboBox(this);
    m_sourceBox->addItem("Mode DÉMO (simulateur)", QVariant::fromValue(m_demo));
    m_sourceBox->addItem("Mode MQTT (nœuds réels)", QVariant::fromValue(m_mqtt));
    m_brokerIp = new QLineEdit("127.0.0.1", this);
    m_brokerIp->setPlaceholderText("IP broker");
    m_brokerIp->setFixedWidth(130);
    m_brokerPort = new QLineEdit("1884", this);
    m_brokerPort->setFixedWidth(56);
    m_btnConnecter = new QPushButton("Connecter", this);
    m_btnConnecter->setEnabled(false);

    auto* barre = new QToolBar(this);
    barre->setMovable(false);
    barre->setStyleSheet("QToolBar{background:#F5F5F5;border:none;border-bottom:1px solid #D0D0D0;spacing:6px;padding:4px;}");
    barre->addWidget(new QLabel("Source :", this));
    barre->addWidget(m_sourceBox);
    barre->addSeparator();
    barre->addWidget(new QLabel("Broker :", this));
    barre->addWidget(m_brokerIp);
    barre->addWidget(m_brokerPort);
    barre->addWidget(m_btnConnecter);
    barre->addSeparator();
    auto* btnForcerEvac = new QPushButton("ÉVACUATION GÉNÉRALE", this);
    btnForcerEvac->setObjectName("btnEvacuation");
    barre->addWidget(btnForcerEvac);
    addToolBar(barre);

    // --- Zone centrale ----------------------------------------------------
    m_status = new StatusBar(this);
    m_grid = new SalleGrid(this);
    m_detail = new SalleDetail(this);
    m_alertPanel = new AlertPanel(this);

    m_log = new QPlainTextEdit(this);
    m_log->setObjectName("logTexte");
    m_log->setReadOnly(true);
    m_log->setMaximumHeight(110);

    auto* colGauche = new QWidget(this);
    auto* layGauche = new QVBoxLayout(colGauche);
    layGauche->setContentsMargins(0, 0, 0, 0);
    layGauche->setSpacing(6);
    layGauche->addWidget(m_status);
    layGauche->addWidget(m_grid, 1);

    auto* colCentre = new QWidget(this);
    auto* layCentre = new QVBoxLayout(colCentre);
    layCentre->setContentsMargins(0, 0, 0, 0);
    layCentre->setSpacing(6);
    layCentre->addWidget(m_detail, 1);
    layCentre->addWidget(m_log);

    auto* split = new QSplitter(Qt::Horizontal, this);
    split->addWidget(colGauche);
    split->addWidget(colCentre);
    split->addWidget(m_alertPanel);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 4);
    split->setStretchFactor(2, 3);
    split->setSizes({360, 700, 380});
    setCentralWidget(split);

    // --- Liaisons source -------------------------------------------------
    basculerSource(m_demo);

    connect(m_sourceBox, &QComboBox::currentIndexChanged,
            this, &MainWindow::onSourceChange);
    connect(m_btnConnecter, &QPushButton::clicked,
            this, &MainWindow::connecterMqtt);
    connect(btnForcerEvac, &QPushButton::clicked, this, [this]() {
        m_source->forcerEvacuation(QString(), true);
    });

    // --- Liaisons IHM ------------------------------------------------------
    connect(m_grid, &SalleGrid::salleSelectionnee, this,
            [this](const QString& id) {
                if (m_salles.contains(id))
                    m_detail->afficherSalle(m_salles[id]);
            });
    connect(m_detail, &SalleDetail::configDemandee,
            m_source, &DataSource::envoyerConfig);
    connect(m_detail, &SalleDetail::testDemande,
            m_source, &DataSource::commanderTest);
    connect(m_detail, &SalleDetail::evacuationForcee,
            m_source, &DataSource::forcerEvacuation);
    connect(m_detail, &SalleDetail::resetAlertes,
            m_source, &DataSource::resetAlertesSalle);
    connect(m_detail, &SalleDetail::alerteAcquittee,
            this, &MainWindow::acquitter);
    connect(m_alertPanel, &AlertPanel::alerteAcquittee,
            this, &MainWindow::acquitter);
    connect(m_alertPanel, &AlertPanel::voirDetail, this,
            [this](const QString& salleId, quint64) {
                if (m_salles.contains(salleId)) {
                    m_detail->afficherSalle(m_salles[salleId]);
                    m_detail->ouvrirAlertes();
                }
            });
}

MainWindow::~MainWindow()
{
    if (m_source)
        m_source->stop();
}

void MainWindow::basculerSource(DataSource* src)
{
    if (m_source && m_source != src)
        m_source->stop();
    m_source = src;

    connect(src, &DataSource::salleMiseAJour, this, &MainWindow::onSalleMiseAJour);
    connect(src, &DataSource::alerteAjoutee, this, &MainWindow::onAlerteAjoutee);
    connect(src, &DataSource::alerteModifiee, this, &MainWindow::onAlerteModifiee);
    connect(src, &DataSource::statutMqtt, m_status, &StatusBar::setStatutMqtt);
    connect(src, &DataSource::statutAsterisk, m_status, &StatusBar::setStatutAsterisk);
    connect(src, &DataSource::noeudsMaj, m_status, &StatusBar::setNoeuds);
    connect(src, &DataSource::evacuationGlobale, this, &MainWindow::onEvacuationGlobale);
    connect(src, &DataSource::configConfirmee, this, &MainWindow::onConfigConfirmee);
    connect(src, &DataSource::testRetour, this, &MainWindow::onTestRetour);
    connect(src, &DataSource::logAppend, this, [this](const QString& t) {
        m_log->appendPlainText(QTime::currentTime().toString("hh:mm:ss") + "  " + t);
    });
    src->start();
}

void MainWindow::onSourceChange()
{
    auto* src = m_sourceBox->currentData().value<DataSource*>();
    if (!src)
        return;
    m_log->clear();
    m_detail->viderAlertes();
    m_salles.clear();
    m_alertPanel->vider();
    m_btnConnecter->setEnabled(src == m_mqtt);
}

void MainWindow::connecterMqtt()
{
    const quint16 port = m_brokerPort->text().toUShort();
    m_mqtt->connecter(m_brokerIp->text(), port, "qt_dashboard");
}

void MainWindow::onSalleMiseAJour(const QString& id)
{
    const Salle s = m_source->salles().value(id);
    m_salles.insert(id, s);
    m_grid->majSalle(s);
}

void MainWindow::onAlerteAjoutee(const Alerte& a)
{
    m_detail->ajouterAlerte(a);
    m_alertPanel->ajouterAlerte(a);
    if (a.severite() == "critique")
        m_detail->afficherSalle(m_salles.value(a.salleId));
}

void MainWindow::onAlerteModifiee(const Alerte& a)
{
    m_detail->modifierAlerte(a);
    m_alertPanel->modifierAlerte(a);
}

void MainWindow::onConfigConfirmee(const QString& salleId, const QString& detail,
                                   int latenceMs)
{
    m_detail->configConfirmee(salleId, detail, latenceMs);
}

void MainWindow::onTestRetour(const QString& salleId, const QString& composant,
                              bool ok, int latenceMs)
{
    m_detail->testRetour(salleId, composant, ok, latenceMs);
}

void MainWindow::onEvacuationGlobale(bool active)
{
    m_evacGlobale = active;
    m_status->setEvacuationGlobale(active);
}

void MainWindow::acquitter(const QString&, quint64 ts)
{
    m_source->acquitterAlerte(ts);
}
