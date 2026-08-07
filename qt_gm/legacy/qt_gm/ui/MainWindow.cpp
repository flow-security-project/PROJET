#include "MainWindow.h"

#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTime>
#include <QToolBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>

#include "engine/GmSource.h"
#include "engine/demo/DemoGmSource.h"
#include "engine/mqtt/MqttGmSource.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("i++ v4.0 — Supervision des salles (GM)");
    resize(900, 620);

    m_demo = new DemoGmSource(this);
    m_mqtt = new MqttGmSource(this);
    m_source = m_demo;

    // --- Barre de source -------------------------------------------------
    m_sourceBox = new QComboBox(this);
    m_sourceBox->addItem("Mode DÉMO (salles simulées)", QVariant::fromValue(m_demo));
    m_sourceBox->addItem("Mode MQTT (salles du binôme)", QVariant::fromValue(m_mqtt));
    m_brokerIp = new QLineEdit("127.0.0.1", this);
    m_brokerIp->setPlaceholderText("IP broker");
    m_brokerIp->setFixedWidth(130);
    m_brokerPort = new QLineEdit("1883", this);
    m_brokerPort->setFixedWidth(56);
    m_btnConnecter = new QPushButton("Connecter", this);
    m_btnConnecter->setEnabled(false);

    auto* barre = new QToolBar(this);
    barre->setMovable(false);
    barre->setStyleSheet(
        "QToolBar{background:#F5F5F5;border:none;border-bottom:1px solid #D0D0D0;"
        "spacing:6px;padding:4px;}");
    barre->addWidget(new QLabel("Source :", this));
    barre->addWidget(m_sourceBox);
    barre->addSeparator();
    barre->addWidget(new QLabel("Broker :", this));
    barre->addWidget(m_brokerIp);
    barre->addWidget(m_brokerPort);
    barre->addWidget(m_btnConnecter);
    addToolBar(barre);

    // --- Zone centrale ----------------------------------------------------
    m_boutonsHote = new QWidget(this);
    auto* grille = new QGridLayout(m_boutonsHote);
    grille->setContentsMargins(10, 10, 10, 10);
    grille->setSpacing(12);

    auto* entete = new QLabel(
        "Salles créées par le binôme — cliquer sur une salle pour ouvrir son interface",
        this);
    entete->setStyleSheet("font-size:12px;color:#555555;");

    m_log = new QPlainTextEdit(this);
    m_log->setObjectName("logTexte");
    m_log->setReadOnly(true);
    m_log->setMaximumHeight(90);

    auto* colGauche = new QWidget(this);
    auto* layGauche = new QVBoxLayout(colGauche);
    layGauche->setContentsMargins(0, 0, 0, 0);
    layGauche->setSpacing(6);
    layGauche->addWidget(entete);
    layGauche->addWidget(m_boutonsHote, 1);

    auto* split = new QSplitter(Qt::Vertical, this);
    split->addWidget(colGauche);
    split->addWidget(m_log);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 0);
    setCentralWidget(split);

    // --- Liaisons ---------------------------------------------------------
    basculerSource(m_demo);

    connect(m_sourceBox, &QComboBox::currentIndexChanged,
            this, &MainWindow::onSourceChange);
    connect(m_btnConnecter, &QPushButton::clicked,
            this, &MainWindow::connecterMqtt);
}

MainWindow::~MainWindow()
{
    if (m_source)
        m_source->stop();
}

void MainWindow::basculerSource(GmSource* src)
{
    if (m_source && m_source != src) {
        m_source->stop();
        disconnect(m_source, nullptr, this, nullptr);
    }
    m_source = src;

    connect(src, &GmSource::salleAjoutee, this, &MainWindow::onSalleAjoutee);
    connect(src, &GmSource::salleMiseAJour, this, &MainWindow::onSalleMiseAJour);
    connect(src, &GmSource::logAppend, this, [this](const QString& t) {
        m_log->appendPlainText(QTime::currentTime().toString("hh:mm:ss") + "  " + t);
    });
    src->start();
}

void MainWindow::onSourceChange()
{
    auto* src = m_sourceBox->currentData().value<GmSource*>();
    if (!src)
        return;
    m_log->clear();
    for (SalleWindow* w : m_fenetres)
        w->close();
    m_fenetres.clear();
    for (QPushButton* b : m_boutons)
        b->deleteLater();
    m_boutons.clear();
    m_salles.clear();

    delete m_boutonsHote->layout();
    auto* grille = new QGridLayout(m_boutonsHote);
    grille->setContentsMargins(10, 10, 10, 10);
    grille->setSpacing(12);

    m_btnConnecter->setEnabled(src == m_mqtt);
    basculerSource(src);
}

void MainWindow::connecterMqtt()
{
    const quint16 port = m_brokerPort->text().toUShort();
    m_mqtt->connecter(m_brokerIp->text(), port, "qt_gm");
}

void MainWindow::ouvrirSalleDepuisArguments(const SalleGm& salle,
                                            const QString& fichierSynchronisation)
{
    m_demo->ajouterSalle(salle);
    if (!fichierSynchronisation.isEmpty())
        m_demo->activerSynchronisation(fichierSynchronisation);
    QTimer::singleShot(150, this, [this, id = salle.id]() {
        ouvrirSalle(id);
    });
}

void MainWindow::onSalleAjoutee(const QString& id)
{
    m_salles.insert(id, m_source->salles().value(id));
    construireBouton(id);
}

void MainWindow::onSalleMiseAJour(const QString& id)
{
    m_salles.insert(id, m_source->salles().value(id));
    majBouton(id);
}

void MainWindow::construireBouton(const QString& id)
{
    auto* bouton = new QPushButton(m_boutonsHote);
    bouton->setObjectName("btnSalle");
    bouton->setFixedSize(210, 96);
    bouton->setCursor(Qt::PointingHandCursor);
    bouton->setStyleSheet(
        "QPushButton#btnSalle{background:#FFFFFF;border:1px solid #D0D0D0;"
        "border-radius:2px;text-align:left;padding:8px;}"
        "QPushButton#btnSalle:hover{border-color:#4A90D9;background:#FAFAFA;}");
    bouton->setText("—");
    connect(bouton, &QPushButton::clicked, this, [this, id]() { ouvrirSalle(id); });

    auto* grille = (QGridLayout*)m_boutonsHote->layout();
    const int index = m_boutons.size();
    grille->addWidget(bouton, index / 3, index % 3);

    m_boutons.insert(id, bouton);
    majBouton(id);
}

void MainWindow::majBouton(const QString& id)
{
    if (!m_boutons.contains(id) || !m_salles.contains(id))
        return;
    const SalleGm& s = m_salles[id];
    const QString etat = s.enLigne ? "●" : "○";
    m_boutons[id]->setText(
        QString("%1 %2\nOccupation : %3/%4\nEntrées : %5   |   Sorties : %6")
            .arg(etat,
                 s.nom.isEmpty() ? id : s.nom,
                 QString::number(s.occupation),
                 QString::number(s.capacite),
                 QString::number(s.nbEntrees),
                 QString::number(s.nbSorties)));
}

void MainWindow::ouvrirSalle(const QString& id)
{
    if (m_fenetres.contains(id)) {
        m_fenetres[id]->raise();
        m_fenetres[id]->activateWindow();
        return;
    }
    auto* w = new SalleWindow(m_source, id);
    w->setAttribute(Qt::WA_DeleteOnClose);
    connect(w, &QObject::destroyed, this, [this, id]() { m_fenetres.remove(id); });
    m_fenetres.insert(id, w);
    w->show();
}
