#include "ConfigurationTab.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

ConfigurationTab::ConfigurationTab(QWidget* parent)
    : QWidget(parent)
{
    m_nom = new QLineEdit(this);
    m_capacite = new QSpinBox(this);
    m_capacite->setRange(1, 5000);
    m_seuilEvac = new QSpinBox(this);
    m_seuilEvac->setRange(1, 100);
    m_seuilEvac->setSuffix(" %");
    m_debut = new QDateTimeEdit(this);
    m_debut->setDisplayFormat("hh:mm");
    m_debut->setTime(QTime(8, 0));
    m_fin = new QDateTimeEdit(this);
    m_fin->setDisplayFormat("hh:mm");
    m_fin->setTime(QTime(18, 0));

    m_retour = new QLabel("", this);
    m_retour->setStyleSheet("font-size:10px;color:#555555;");

    auto* form = new QFormLayout;
    form->addRow("Nom", m_nom);
    form->addRow("Capacité (personnes)", m_capacite);
    form->addRow("Seuil évacuation", m_seuilEvac);
    form->addRow("Ouverture", m_debut);
    form->addRow("Fermeture", m_fin);

    auto* btnEnvoyer = new QPushButton("ENVOYER LA CONFIGURATION", this);
    btnEnvoyer->setObjectName("btnPrimaire");
    btnEnvoyer->setStyleSheet(
        "QPushButton{background:#1565C0;color:#FFFFFF;border:none;border-radius:2px;"
        "padding:8px 14px;font-size:12px;font-weight:600;} "
        "QPushButton:hover{background:#4A90D9;}");

    auto* boiteForm = new QGroupBox("Configuration de la salle", this);
    boiteForm->setStyleSheet(
        "QGroupBox{font-size:11px;font-weight:600;color:#1A1A1A;border:1px solid #D0D0D0;"
        "border-radius:2px;margin-top:8px;} "
        "QGroupBox::title{subcontrol-origin:margin;left:8px;top:-6px;}");
    auto* layForm = new QVBoxLayout(boiteForm);
    layForm->setContentsMargins(10, 12, 10, 10);
    layForm->addLayout(form);
    layForm->addWidget(btnEnvoyer);
    layForm->addWidget(m_retour);

    auto* btnLed = new QPushButton("Test LED", this);
    auto* btnStrobo = new QPushButton("Stroboscope", this);
    auto* btnLcd = new QPushButton("Test LCD", this);
    auto* btnEvac = new QPushButton("FORCER ÉVACUATION", this);
    btnEvac->setObjectName("btnEvacuation");
    auto* btnReset = new QPushButton("Réinitialiser alertes", this);
    auto* btnRecuperer = new QPushButton("Relire config du nœud", this);
    for (QPushButton* b :
         {btnLed, btnStrobo, btnLcd, btnRecuperer, btnReset}) {
        b->setStyleSheet(
            "QPushButton{background:#FFFFFF;border:1px solid #D0D0D0;border-radius:2px;"
            "padding:6px 10px;font-size:11px;} "
            "QPushButton:hover{border-color:#4A90D9;}");
    }

    auto* boiteTests = new QGroupBox("Tests de maintenance", this);
    boiteTests->setStyleSheet(
        "QGroupBox{font-size:11px;font-weight:600;color:#1A1A1A;border:1px solid #D0D0D0;"
        "border-radius:2px;margin-top:8px;} "
        "QGroupBox::title{subcontrol-origin:margin;left:8px;top:-6px;}");
    auto* layTests = new QGridLayout(boiteTests);
    layTests->setContentsMargins(10, 12, 10, 10);
    layTests->addWidget(btnLed, 0, 0);
    layTests->addWidget(btnStrobo, 0, 1);
    layTests->addWidget(btnLcd, 0, 2);
    layTests->addWidget(btnEvac, 1, 0, 1, 3);
    layTests->addWidget(btnReset, 2, 0);
    layTests->addWidget(btnRecuperer, 2, 1);

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(10);
    lay->addWidget(boiteForm);
    lay->addWidget(boiteTests);
    lay->addStretch();

    connect(btnEnvoyer, &QPushButton::clicked, this, [this]() {
        emit configDemandee(configActuelle());
        m_retour->setText("Config envoyée — en attente de confirmation du nœud…");
    });
    connect(btnLed, &QPushButton::clicked, this,
            [this]() { emit testDemande(m_salleId, "led", "vert"); });
    connect(btnStrobo, &QPushButton::clicked, this,
            [this]() { emit testDemande(m_salleId, "led", "stroboscope"); });
    connect(btnLcd, &QPushButton::clicked, this,
            [this]() { emit testDemande(m_salleId, "lcd", "TEST OK"); });
    connect(btnEvac, &QPushButton::clicked, this,
            [this]() { emit evacuationForcee(m_salleId, true); });
    connect(btnReset, &QPushButton::clicked, this,
            [this]() { emit resetAlertes(m_salleId); });
    connect(btnRecuperer, &QPushButton::clicked, this,
            [this]() { emit testDemande(m_salleId, "recuperer", ""); });
}

void ConfigurationTab::afficherSalle(const Salle& s)
{
    m_salleId = s.id;
    m_nom->setText(s.nom);
    m_capacite->setValue(s.capacite);
    m_seuilEvac->setValue(s.seuilEvacuation);
    if (s.horaireDebut.isEmpty())
        m_debut->setTime(QTime(8, 0));
    else
        m_debut->setTime(QTime::fromString(s.horaireDebut, "hh:mm"));
    if (s.horaireFin.isEmpty())
        m_fin->setTime(QTime(18, 0));
    else
        m_fin->setTime(QTime::fromString(s.horaireFin, "hh:mm"));
}

void ConfigurationTab::configConfirmee(const QString& detail, int latenceMs)
{
    m_retour->setText(QString("%1 (RTT %2 ms)").arg(detail).arg(latenceMs));
}

void ConfigurationTab::testRetour(const QString& composant, bool ok,
                                  int latenceMs)
{
    m_retour->setText(QString("Test %1 : %2 (%3 ms)")
                          .arg(composant)
                          .arg(ok ? "OK" : "ÉCHEC")
                          .arg(latenceMs));
}

Salle ConfigurationTab::configActuelle() const
{
    Salle cfg;
    cfg.id = m_salleId;
    cfg.nom = m_nom->text().trimmed();
    cfg.capacite = m_capacite->value();
    cfg.seuilEvacuation = m_seuilEvac->value();
    cfg.horaireDebut = m_debut->time().toString("hh:mm");
    cfg.horaireFin = m_fin->time().toString("hh:mm");
    return cfg;
}
