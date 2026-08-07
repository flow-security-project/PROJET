#include "SalleDetail.h"

#include <QLabel>
#include <QVBoxLayout>

SalleDetail::SalleDetail(QWidget* parent)
    : QWidget(parent)
{
    m_titre = new QLabel("Salle —", this);
    m_titre->setStyleSheet("font-size:14px;font-weight:700;color:#1A1A1A;");

    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    m_tabs->setStyleSheet(
        "QTabWidget::pane{border:1px solid #D0D0D0;border-radius:2px;}"
        "QTabBar::tab{background:#F5F5F5;color:#555555;padding:6px 14px;"
        "border:1px solid #D0D0D0;border-bottom:none;font-size:11px;}"
        "QTabBar::tab:selected{background:#FFFFFF;color:#1565C0;font-weight:700;}");

    m_visu = new VisualisationTab(this);
    m_config = new ConfigurationTab(this);
    m_alertes = new AlertesTab(this);
    m_tabs->addTab(m_visu, "Visualisation");
    m_tabs->addTab(m_config, "Configuration");
    m_tabs->addTab(m_alertes, "Alertes");

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 6, 8, 8);
    lay->setSpacing(6);
    lay->addWidget(m_titre);
    lay->addWidget(m_tabs, 1);

    connect(m_config, &ConfigurationTab::configDemandee,
            this, &SalleDetail::configDemandee);
    connect(m_config, &ConfigurationTab::testDemande,
            this, &SalleDetail::testDemande);
    connect(m_config, &ConfigurationTab::evacuationForcee,
            this, &SalleDetail::evacuationForcee);
    connect(m_config, &ConfigurationTab::resetAlertes,
            this, &SalleDetail::resetAlertes);
    connect(m_alertes, &AlertesTab::alerteAcquittee,
            this, &SalleDetail::alerteAcquittee);
}

void SalleDetail::afficherSalle(const Salle& s)
{
    m_titre->setText(QString("%1 (%2)").arg(s.nom).arg(s.id));
    m_visu->majSalle(s);
    m_config->afficherSalle(s);
}

void SalleDetail::configConfirmee(const QString&, const QString& detail,
                                  int latenceMs)
{
    m_config->configConfirmee(detail, latenceMs);
}

void SalleDetail::testRetour(const QString&, const QString& composant,
                             bool ok, int latenceMs)
{
    m_config->testRetour(composant, ok, latenceMs);
}

void SalleDetail::ajouterAlerte(const Alerte& a)
{
    m_alertes->ajouterAlerte(a);
    m_tabs->setTabText(2, QString("Alertes (%1)").arg(m_alertes->nbAlertes()));
}

void SalleDetail::modifierAlerte(const Alerte& a)
{
    m_alertes->modifierAlerte(a);
}

void SalleDetail::viderAlertes()
{
    m_alertes->vider();
    m_tabs->setTabText(2, "Alertes");
}

void SalleDetail::ouvrirAlertes()
{
    m_tabs->setCurrentIndex(2);
}
