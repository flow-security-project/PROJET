#include "MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolBar>
#include <QVariant>
#include <QMessageBox>

#include "data/DataSource.h"
#include "data/DemoSource.h"
#include "data/MqttSource.h"
#include "engine/annonce/SpeakManager.h"
#include "engine/appel/AppelManager.h"
#include "ui/SallesWidget.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("i++ — Supervision des salles"));
    resize(1280, 780);

    m_demo = new DemoSource(this);
    m_mqtt = new MqttSource(this);
    m_salles = new SallesWidget(this);

    m_sourceBox = new QComboBox(this);
    m_sourceBox->addItem(QStringLiteral("Mode DÉMO"),
                         QVariant::fromValue(static_cast<DataSource*>(m_demo)));
    m_sourceBox->addItem(QStringLiteral("Mode MQTT"),
                         QVariant::fromValue(static_cast<DataSource*>(m_mqtt)));
    m_brokerIp = new QLineEdit(QStringLiteral("127.0.0.1"), this);
    m_brokerIp->setPlaceholderText(QStringLiteral("IP broker"));
    m_brokerIp->setFixedWidth(140);
    m_brokerPort = new QLineEdit(QStringLiteral("1884"), this);
    m_brokerPort->setFixedWidth(58);
    auto* connectButton = new QPushButton(QStringLiteral("Connecter"), this);
    connectButton->setEnabled(false);

    m_voix = new QCheckBox(QStringLiteral("VOIX"), this);
    m_voix->setChecked(true);
    m_voix->setToolTip(QStringLiteral(
        "Active ou désactive les annonces vocales automatiques "
        "(évacuation, saturation, redirection UNI, attente MULTI, intrusion, "
        "flux de sortie anormal, retour à la normale)."));
    m_testVoix = new QPushButton(QStringLiteral("TEST VOIX"), this);
    m_testVoix->setToolTip(QStringLiteral(
        "Lit un message de test pour vérifier la synthèse vocale."));

    m_testAppel = new QPushButton(QStringLiteral("TEST APPEL"), this);
    m_testAppel->setToolTip(QStringLiteral(
        "Lance un appel de test vers le numéro configuré (ARI + SIP MESSAGE)."));

    m_langueVoix = new QComboBox(this);
    m_langueVoix->addItem(QStringLiteral("FRANÇAIS"), QStringLiteral("fr"));
    m_langueVoix->addItem(QStringLiteral("ENGLISH"), QStringLiteral("en"));
    m_langueVoix->setToolTip(QStringLiteral(
        "Langue globale des annonces vocales (défaut pour les nouvelles "
        "salles, le test et les salles non configurées)."));

    m_asteriskBadge = new QLabel(QStringLiteral("ASTERISK: ⚪"), this);
    m_asteriskBadge->setToolTip(QStringLiteral("État de la connexion Asterisk (ARI/AMI)"));

    auto* toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->addWidget(new QLabel(QStringLiteral("Source :"), this));
    toolbar->addWidget(m_sourceBox);
    toolbar->addSeparator();
    toolbar->addWidget(new QLabel(QStringLiteral("Broker :"), this));
    toolbar->addWidget(m_brokerIp);
    toolbar->addWidget(m_brokerPort);
    toolbar->addWidget(connectButton);
    toolbar->addSeparator();
    toolbar->addWidget(m_voix);
    toolbar->addWidget(m_langueVoix);
    toolbar->addWidget(m_testVoix);
    toolbar->addSeparator();
    toolbar->addWidget(m_asteriskBadge);
    toolbar->addWidget(m_testAppel);
    addToolBar(toolbar);

    setCentralWidget(m_salles);
    m_salles->setSource(m_demo);

    connect(m_sourceBox, &QComboBox::currentIndexChanged,
            this, &MainWindow::onSourceChange);
    connect(connectButton, &QPushButton::clicked,
            this, &MainWindow::connecterMqtt);
    connect(m_sourceBox, &QComboBox::currentIndexChanged,
            this, [connectButton](int index) {
                connectButton->setEnabled(index == 1);
            });
    connect(m_voix, &QCheckBox::toggled, this, [this](bool actif) {
        if (m_salles && m_salles->speakManager())
            m_salles->speakManager()->setActif(actif);
    });
    connect(m_langueVoix, &QComboBox::currentIndexChanged, this, [this](int) {
        if (m_salles && m_salles->speakManager())
            m_salles->speakManager()->setLangueGlobale(
                m_langueVoix->currentData().toString());
    });
    connect(m_testVoix, &QPushButton::clicked, this, [this]() {
        if (m_salles && m_salles->speakManager())
            m_salles->speakManager()->testVoix();
    });
    connect(m_testAppel, &QPushButton::clicked, this, [this]() {
        if (m_salles && m_salles->appelManager())
            m_salles->appelManager()->testAppel();
    });

    // Connexions AppelManager pour mise à jour badge Asterisk
    if (m_salles && m_salles->appelManager()) {
        auto* appel = m_salles->appelManager();
        // On connecte via les signaux publics d'AppelManager
        connect(appel, &AppelManager::log, this, [this](const QString& msg) {
            m_asteriskBadge->setText(msg.contains("connecté", Qt::CaseInsensitive)
                                        ? "ASTERISK: 🟢"
                                        : msg.contains("déconnecté", Qt::CaseInsensitive)
                                            ? "ASTERISK: 🔴"
                                            : "ASTERISK: 🟡");
        });
    }
}

MainWindow::~MainWindow()
{
    if (m_salles)
        m_salles->setSource(nullptr);
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange && !m_basculePleinEcran) {
        const Qt::WindowStates etat = windowState();
        if (!m_restauration
            && etat.testFlag(Qt::WindowMaximized)
            && !etat.testFlag(Qt::WindowFullScreen)) {
            m_etatAvantPleinEcran = etat;
            m_basculePleinEcran = true;
            showFullScreen();
            m_basculePleinEcran = false;
        }
        m_restauration = false;
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape && isFullScreen()) {
        m_restauration = true;
        setWindowState(m_etatAvantPleinEcran);
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::onSourceChange()
{
    auto* source = m_sourceBox->currentData().value<DataSource*>();
    if (source)
        m_salles->setSource(source);
}

void MainWindow::connecterMqtt()
{
    m_mqtt->connecter(m_brokerIp->text().trimmed(),
                      m_brokerPort->text().toUShort(),
                      QStringLiteral("salles_manager"));
}
