#include "SallesWidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QTime>
#include <QVBoxLayout>

#include "data/DataSource.h"
#include "ui/SalleConfigWidget.h"
#include "ui/SalleDetailWidget.h"
#include "ui/SalleGrid.h"

SallesWidget::SallesWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("sallesRoot"));
    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("statusLabel"));
    m_status->setText(QStringLiteral("Aucune source active"));

    m_grid = new SalleGrid(this);
    m_config = new SalleConfigWidget(this);

    auto* left = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(new QLabel(QStringLiteral("Vue globale des salles"), left));

    auto* scroll = new QScrollArea(left);
    scroll->setObjectName(QStringLiteral("sallesScroll"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(m_grid);
    leftLayout->addWidget(scroll, 1);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(left);
    splitter->addWidget(m_config);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({760, 360});

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(5);
    layout->addWidget(m_status);
    layout->addWidget(splitter, 1);

    connect(m_grid, &SalleGrid::salleSelectionnee,
            this, &SallesWidget::onSalleSelectionnee);
    connect(m_config, &SalleConfigWidget::creerDemandee,
            this, &SallesWidget::onCreerDemandee);
    connect(m_config, &SalleConfigWidget::modificationDemandee,
            this, &SallesWidget::onModificationDemandee);
    connect(m_config, &SalleConfigWidget::mesureDemandee,
            this, &SallesWidget::onMesureDemandee);
    connect(m_config, &SalleConfigWidget::actualisationDemandee,
            this, &SallesWidget::onActualisationDemandee);
    connect(m_config, &SalleConfigWidget::masquageDemande,
            this, &SallesWidget::onMasquageDemande);
    connect(m_config, &SalleConfigWidget::restaurationDemandee,
            this, &SallesWidget::onRestaurationDemandee);
    connect(m_config, &SalleConfigWidget::courbeDemandee,
            this, &SallesWidget::onCourbeDemandee);
    connect(m_config, &SalleConfigWidget::nouvelleDemandee,
            this, [this]() {
                m_selectionId.clear();
                m_config->afficherCreation();
            });
}

void SallesWidget::setSource(DataSource* source)
{
    if (m_source == source)
        return;
    if (m_detailDialog)
        m_detailDialog->close();
    m_detailSalleId.clear();
    m_selectionId.clear();
    if (m_source) {
        disconnect(m_source, nullptr, this, nullptr);
        m_source->stop();
    }

    m_source = source;
    m_grid->viderVue();
    m_config->afficherCreation();
    if (!m_source)
        return;

    connecterSource(m_source);
    m_source->start();
    synchroniserSalles();
}

void SallesWidget::onSalleAjoutee(const QString& id)
{
    if (!m_source || !m_source->salles().contains(id))
        return;
    const Salle salle = m_source->salles().value(id);
    m_grid->majSalle(salle);
    actualiserListeMasquees();
    if (id == m_selectionId)
        m_config->afficherSalle(salle);
    emit statutChanged(QStringLiteral("Salle %1 ajoutée").arg(id));
}

void SallesWidget::onSalleMiseAJour(const QString& id)
{
    if (!m_source || !m_source->salles().contains(id))
        return;
    const Salle salle = m_source->salles().value(id);
    m_grid->majSalle(salle);
    actualiserListeMasquees();
    emit statutChanged(QStringLiteral("Données actualisées — %1").arg(id));
}

void SallesWidget::onSalleSelectionnee(const QString& id)
{
    if (!m_source || !m_source->salles().contains(id))
        return;
    m_selectionId = id;
    m_config->afficherSalle(m_source->salles().value(id));
}

void SallesWidget::onCreerDemandee(const Salle& salle)
{
    if (!m_source)
        return;
    m_selectionId = salle.id;
    m_source->creerSalle(salle);
}

void SallesWidget::onModificationDemandee(const Salle& salle)
{
    if (!m_source)
        return;
    m_source->modifierSalle(salle);
}

void SallesWidget::onMesureDemandee(const QString& id)
{
    if (!m_source)
        return;
    m_source->getHauteurPorte(id);
}

void SallesWidget::onActualisationDemandee(const QString& id)
{
    if (!m_source)
        return;
    m_source->actualiserSalle(id);
}

void SallesWidget::onMasquageDemande(const QString& id)
{
    if (id.isEmpty())
        return;
    m_grid->masquerSalle(id);
    m_selectionId.clear();
    m_config->afficherCreation();
    actualiserListeMasquees();
    emit statutChanged(QStringLiteral("Carte masquée — %1 reste conservée").arg(id));
}

void SallesWidget::onRestaurationDemandee(const QString& id)
{
    if (id.isEmpty()) {
        m_config->afficherErreur(QStringLiteral("Sélectionnez une salle masquée."));
        return;
    }
    if (!m_grid->restaurerSalle(id)) {
        m_config->afficherErreur(QStringLiteral("Impossible de réafficher la salle %1.").arg(id));
        return;
    }
    actualiserListeMasquees();
    m_selectionId = id;
    if (m_source && m_source->salles().contains(id))
        m_config->afficherSalle(m_source->salles().value(id));
    emit statutChanged(QStringLiteral("Carte réaffichée — %1").arg(id));
}

void SallesWidget::onCourbeDemandee(const Salle& salle)
{
    if (!m_source || !m_source->salles().contains(salle.id))
        return;

    if (m_detailDialog && m_detailSalleId == salle.id) {
        m_detailDialog->raise();
        m_detailDialog->activateWindow();
        return;
    }
    if (m_detailDialog) {
        m_detailDialog->close();
        m_detailDialog = nullptr;
    }

    m_detailDialog = new QDialog(this);
    m_detailSalleId = salle.id;
    m_detailDialog->setAttribute(Qt::WA_DeleteOnClose);
    m_detailDialog->setWindowTitle(QStringLiteral("Détail salle — %1")
                                       .arg(salle.nom.isEmpty() ? salle.id : salle.nom));
    m_detailDialog->resize(980, 700);

    auto* detail = new SalleDetailWidget(m_source, salle.id, m_detailDialog);
    auto* close = new QDialogButtonBox(QDialogButtonBox::Close, m_detailDialog);
    connect(close, &QDialogButtonBox::rejected,
            m_detailDialog, &QDialog::reject);
    auto* layout = new QVBoxLayout(m_detailDialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(detail, 1);
    layout->addWidget(close);
    connect(m_detailDialog, &QObject::destroyed, this, [this]() {
        m_detailDialog = nullptr;
        m_detailSalleId.clear();
    });
    m_detailDialog->show();
}

void SallesWidget::onSourceErreur(const QString& message)
{
    m_config->afficherErreur(message);
    emit statutChanged(message);
}

void SallesWidget::onSourceLog(const QString& message)
{
    m_status->setText(QTime::currentTime().toString(QStringLiteral("hh:mm:ss"))
                      + QStringLiteral("  ") + message);
}

void SallesWidget::onHauteurMesuree(const QString& id, double centimetres,
                                    bool succes, const QString& note)
{
    m_config->afficherMesure(id, centimetres, succes, note);
    if (succes)
        emit statutChanged(QStringLiteral("Hauteur mesurée — %1 : %2 cm")
                               .arg(id)
                               .arg(centimetres, 0, 'f', 1));
}

void SallesWidget::connecterSource(DataSource* source)
{
    connect(source, &DataSource::salleAjoutee,
            this, &SallesWidget::onSalleAjoutee);
    connect(source, &DataSource::salleMiseAJour,
            this, &SallesWidget::onSalleMiseAJour);
    connect(source, &DataSource::hauteurPorteMesuree,
            this, &SallesWidget::onHauteurMesuree);
    connect(source, &DataSource::erreur,
            this, &SallesWidget::onSourceErreur);
    connect(source, &DataSource::logAppend,
            this, &SallesWidget::onSourceLog);
    connect(source, &DataSource::statutSource, this,
            [this](bool connecte, const QString& note) {
                m_status->setText(QStringLiteral("Source : %1 — %2")
                                      .arg(connecte ? QStringLiteral("OK")
                                                    : QStringLiteral("ATTENTE"),
                                           note));
                emit statutChanged(note);
            });
}

void SallesWidget::synchroniserSalles()
{
    if (!m_source)
        return;
    for (const Salle& salle : m_source->salles())
        m_grid->majSalle(salle);
    actualiserListeMasquees();
}

void SallesWidget::actualiserListeMasquees()
{
    if (m_source)
        m_config->setSallesMasquees(m_source->salles(), m_grid->sallesMasquees());
}
