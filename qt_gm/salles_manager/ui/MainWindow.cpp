#include "MainWindow.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolBar>
#include <QVariant>

#include "data/DataSource.h"
#include "data/DemoSource.h"
#include "data/MqttSource.h"
#include "ui/SallesWidget.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("i++ — Création et supervision des salles"));
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

    auto* toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->addWidget(new QLabel(QStringLiteral("Source :"), this));
    toolbar->addWidget(m_sourceBox);
    toolbar->addSeparator();
    toolbar->addWidget(new QLabel(QStringLiteral("Broker :"), this));
    toolbar->addWidget(m_brokerIp);
    toolbar->addWidget(m_brokerPort);
    toolbar->addWidget(connectButton);
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
}

MainWindow::~MainWindow()
{
    if (m_salles)
        m_salles->setSource(nullptr);
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
