#include "SallesWidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTime>
#include <QVBoxLayout>

#include "data/DataSource.h"
#include "history/HistoryManager.h"
#include "models/AlerteModel.h"
#include "ui/AlertePanelWidget.h"
#include "ui/SalleConfigWidget.h"
#include "ui/SalleDetailWidget.h"
#include "ui/SalleGrid.h"
#include "ui/StadeWidget.h"

SallesWidget::SallesWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("sallesRoot"));
    setAttribute(Qt::WA_StyledBackground, true);
    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("statusLabel"));
    m_status->setText(QStringLiteral("Aucune source active"));

    m_grid = new SalleGrid(this);
    m_config = new SalleConfigWidget(this);
    m_history = new HistoryManager(QString(), this);
    m_modeleAlertes = new AlerteModel(this);
    m_modeleAlertes->charger(m_history->alertes());
    m_panelAlertes = new AlertePanelWidget(m_modeleAlertes, m_history, this);

    connect(m_modeleAlertes, &AlerteModel::alerteAjoutee, this,
            [this](const Alerte& alerte) { m_history->recordAlerte(alerte); });
    connect(m_modeleAlertes, &AlerteModel::alerteModifiee, this,
            [this](const Alerte& alerte) { m_history->updateAlerte(alerte); });
    connect(m_history, &HistoryManager::storageError, this,
            [this](const QString& message) { emit statutChanged(message); });

    auto* left = new QWidget(this);
    left->setObjectName(QStringLiteral("sallesLeft"));
    left->setAttribute(Qt::WA_StyledBackground, true);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    auto* titreGlobal = new QLabel(QStringLiteral("Vue globale des salles"), left);
    titreGlobal->setObjectName(QStringLiteral("pageTitle"));
    leftLayout->addWidget(titreGlobal);

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

    auto* splitterVertical = new QSplitter(Qt::Vertical, this);
    splitterVertical->addWidget(splitter);
    splitterVertical->addWidget(m_panelAlertes);
    splitterVertical->setStretchFactor(0, 4);
    splitterVertical->setStretchFactor(1, 1);
    splitterVertical->setSizes({560, 190});

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(5);

    auto* header = new QHBoxLayout;
    header->setContentsMargins(0, 0, 0, 0);
    header->addWidget(m_status);
    header->addStretch();
    m_boutonDemoRapide = new QPushButton(
        QStringLiteral("Salles de démonstration"), this);
    m_boutonDemoRapide->setObjectName(QStringLiteral("btnDemoRapide"));
    m_boutonDemoRapide->setToolTip(QStringLiteral(
        "Crée instantanément 5 salles fictives animées (Amphi A, Labo Info, "
        "TD Maths, Salle Réunion, Salle Évacuée) pour présenter la "
        "supervision en mode démo."));
    m_boutonDemoRapide->setVisible(false);
    connect(m_boutonDemoRapide, &QPushButton::clicked,
            this, &SallesWidget::onDemoRapide);
    header->addWidget(m_boutonDemoRapide);

    layout->addLayout(header);
    layout->addWidget(splitterVertical, 1);

    connect(m_grid, &SalleGrid::salleSelectionnee,
            this, &SallesWidget::onSalleSelectionnee);
    connect(m_grid, &SalleGrid::groupeSelectionnee,
            this, &SallesWidget::onGroupeSelectionnee);
    connect(m_config, &SalleConfigWidget::creerDemandee,
            this, &SallesWidget::onCreerDemandee);
    connect(m_config, &SalleConfigWidget::groupeCreerDemande,
            this, &SallesWidget::onGroupeCreerDemande);
    connect(m_config, &SalleConfigWidget::modificationDemandee,
            this, &SallesWidget::onModificationDemandee);
    connect(m_config, &SalleConfigWidget::mesureDemandee,
            this, &SallesWidget::onMesureDemandee);
    connect(m_config, &SalleConfigWidget::actualisationDemandee,
            this, &SallesWidget::onActualisationDemandee);
    connect(m_config, &SalleConfigWidget::masquageDemande,
            this, &SallesWidget::onMasquageDemande);
    connect(m_config, &SalleConfigWidget::suppressionDemandee,
            this, &SallesWidget::onSuppressionDemande);
    connect(m_config, &SalleConfigWidget::restaurationDemandee,
            this, &SallesWidget::onRestaurationDemandee);
    connect(m_config, &SalleConfigWidget::courbeDemandee,
            this, &SallesWidget::onCourbeDemandee);
    connect(m_panelAlertes, &AlertePanelWidget::voirDetailAlerte,
            this, &SallesWidget::onVoirDetailAlerte);
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
    m_history->resetLive();
    m_grid->viderVue();
    m_config->afficherCreation();
    if (!m_source) {
        m_boutonDemoRapide->setVisible(false);
        return;
    }

    connecterSource(m_source);
    m_source->start();
    synchroniserSalles();
    m_boutonDemoRapide->setVisible(m_source->type() == QStringLiteral("demo"));
}

void SallesWidget::onSalleAjoutee(const QString& id)
{
    if (!m_source || !m_source->salles().contains(id))
        return;
    const Salle salle = m_source->salles().value(id);
    m_history->recordSalle(salle);
    m_groupeSalle.insert(id, salle.groupeId);
    if (salle.groupeId.isEmpty())
        m_grid->majSalle(salle);
    actualiserListeMasquees();
    if (!salle.groupeId.isEmpty())
        mettreAJourGroupe(salle.groupeId);
    if (id == m_selectionId)
        m_config->afficherSalle(salle);
    emit statutChanged(QStringLiteral("Salle %1 ajoutée").arg(id));
}

void SallesWidget::onSalleMiseAJour(const QString& id)
{
    if (!m_source || !m_source->salles().contains(id))
        return;
    const Salle salle = m_source->salles().value(id);
    m_history->recordSalle(salle);
    m_groupeSalle.insert(id, salle.groupeId);
    if (salle.groupeId.isEmpty())
        m_grid->majSalle(salle);
    actualiserListeMasquees();
    if (!salle.groupeId.isEmpty())
        mettreAJourGroupe(salle.groupeId);
    if (id == m_selectionId)
        m_config->afficherStatutReseau(salle);
    emit statutChanged(QStringLiteral("Données actualisées — %1").arg(id));
}

void SallesWidget::onSalleSelectionnee(const QString& id)
{
    if (!m_source || !m_source->salles().contains(id))
        return;
    m_selectionId = id;
    m_config->afficherSalle(m_source->salles().value(id));
}

void SallesWidget::onGroupeAjoute(const QString& id)
{
    if (!m_source)
        return;
    mettreAJourGroupe(id);
    emit statutChanged(QStringLiteral("Stade %1 créé").arg(id));
}

void SallesWidget::onGroupeSupprime(const QString& id)
{
    m_grid->supprimerGroupe(id);
    for (auto it = m_groupeSalle.begin(); it != m_groupeSalle.end();) {
        if (it.value() == id)
            it = m_groupeSalle.erase(it);
        else
            ++it;
    }
    if (m_stadeDialog && m_stadeDialog->objectName() == id) {
        m_stadeDialog->close();
        m_stadeDialog = nullptr;
    }
    if (m_selectionId == id) {
        m_selectionId.clear();
        m_config->afficherCreation();
    }
    emit statutChanged(QStringLiteral("Stade supprimé — %1").arg(id));
}

void SallesWidget::onGroupeMiseAJour(const QString& id)
{
    if (!m_source)
        return;
    mettreAJourGroupe(id);
    emit statutChanged(QStringLiteral("Stade modifié — %1").arg(id));
}

void SallesWidget::onGroupeSelectionnee(const QString& id)
{
    if (!m_source || !m_source->groupes().contains(id))
        return;
    ouvrirStade(id);
}

void SallesWidget::onGroupeCreerDemande(const Groupe& groupe)
{
    if (!m_source)
        return;
    if (m_source->groupes().contains(groupe.id)) {
        m_config->afficherErreur(QStringLiteral("Un stade avec l'identifiant %1 existe déjà.")
                                     .arg(groupe.id));
        return;
    }
    if (m_source->salles().contains(groupe.id)) {
        m_config->afficherErreur(QStringLiteral("L'identifiant %1 est déjà utilisé par une salle.")
                                     .arg(groupe.id));
        return;
    }
    m_source->creerGroupe(groupe);
}

void SallesWidget::onCreerDemandee(const Salle& salle)
{
    if (!m_source)
        return;
    if (m_source->groupes().contains(salle.id)) {
        m_config->afficherErreur(QStringLiteral("L'identifiant %1 est déjà utilisé par un stade.")
                                     .arg(salle.id));
        return;
    }
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

void SallesWidget::onSuppressionDemande(const QString& id)
{
    confirmerSuppressionSalle(id);
}

void SallesWidget::onSalleSupprimee(const QString& id)
{
    const QString groupeId = m_groupeSalle.take(id);
    m_grid->supprimerSalle(id);
    actualiserListeMasquees();
    if (!groupeId.isEmpty())
        mettreAJourGroupe(groupeId);
    if (m_detailDialog && m_detailSalleId == id) {
        m_detailDialog->close();
        m_detailDialog = nullptr;
        m_detailSalleId.clear();
    }
    if (m_selectionId == id) {
        m_selectionId.clear();
        m_config->afficherCreation();
    }
    emit statutChanged(QStringLiteral("Salle supprimée — %1").arg(id));
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

    auto* detail = new SalleDetailWidget(m_source, salle.id, m_modeleAlertes,
                                          m_history, m_detailDialog);
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

void SallesWidget::onAlerte(const Alerte& alerte)
{
    m_modeleAlertes->ajouter(alerte);
    emit statutChanged(QStringLiteral("ALERTE — %1 (%2)")
                           .arg(alerte.salleId, alerte.typeLibelle()));
}

void SallesWidget::onVoirDetailAlerte(const QString& salleId, quint64 ts)
{
    if (!m_source || !m_source->salles().contains(salleId))
        return;
    m_grid->selectionnerSalle(salleId);
    onCourbeDemandee(m_source->salles().value(salleId));
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

void SallesWidget::onPassageValide(const QString& salleId, const QString& direction,
                                   qint64 timestampMs)
{
    if (!m_source || !m_source->salles().contains(salleId))
        return;
    m_history->recordPassage(salleId, direction, timestampMs,
                             m_source->salles().value(salleId));
}

void SallesWidget::onDemoRapide()
{
    if (!m_source || m_source->type() != QStringLiteral("demo"))
        return;

    struct ModeleDemo
    {
        QString nom;
        int capacite = 30;
        int scenario = 0;   // 0 lente · 1 rapide · 2 va-et-vient · 4 F3 · 5 F11
        bool evacuation = false;
    };
    const QList<ModeleDemo> modeles = {
        { QStringLiteral("Amphi A"),        120, 1,  false },
        { QStringLiteral("Labo Info"),       30, 0,  false },
        { QStringLiteral("TD Maths"),        30, 2,  false },
        { QStringLiteral("Salle Réunion"),   12, 4,  false },
        { QStringLiteral("Salle Évacuée"),  130, 1,  true  },
    };

    // Recherche d'un id libre dont le scénario démo correspond au modèle
    // (la démo choisit le scénario via qHash(id) % 6).
    auto chercherId = [this](int scenario, int depart) -> QString {
        for (int i = depart; i < depart + 80; ++i) {
            const QString candidat = QStringLiteral("B%1").arg(i, 3, 10, QLatin1Char('0'));
            if (int(qHash(candidat)) % 6 != scenario)
                continue;
            if (!m_source->salles().contains(candidat))
                return candidat;
        }
        return QString();
    };

    int creees = 0;
    int ignorees = 0;
    for (const ModeleDemo& modele : modeles) {
        QString id = chercherId(modele.scenario, 200);
        if (id.isEmpty() || m_source->salles().contains(id)) {
            ++ignorees;
            continue;
        }
        Salle s;
        s.id = id;
        s.nom = modele.nom;
        s.capacite = modele.capacite;
        s.horaireDebut = QStringLiteral("07:00");
        s.horaireFin = QStringLiteral("22:00");
        s.hauteurPorteMesuree = true;
        s.hauteurPorteCm = 210.0;
        s.evacuationActive = modele.evacuation;
        m_source->creerSalle(s);
        ++creees;
    }

    emit statutChanged(creees > 0
                           ? QStringLiteral("Démo rapide : %1 salle(s) créée(s) (%2 déjà "
                                            "présente(s))")
                                 .arg(creees)
                                 .arg(ignorees)
                           : QStringLiteral("Démo rapide : toutes les salles sont déjà "
                                            "présentes"));
}

void SallesWidget::connecterSource(DataSource* source)
{
    connect(source, &DataSource::salleAjoutee,
            this, &SallesWidget::onSalleAjoutee);
    connect(source, &DataSource::salleSupprimee,
            this, &SallesWidget::onSalleSupprimee);
    connect(source, &DataSource::salleMiseAJour,
            this, &SallesWidget::onSalleMiseAJour);
    connect(source, &DataSource::groupeAjoute,
            this, &SallesWidget::onGroupeAjoute);
    connect(source, &DataSource::groupeSupprime,
            this, &SallesWidget::onGroupeSupprime);
    connect(source, &DataSource::groupeMiseAJour,
            this, &SallesWidget::onGroupeMiseAJour);
    connect(source, &DataSource::hauteurPorteMesuree,
            this, &SallesWidget::onHauteurMesuree);
    connect(source, &DataSource::passageValide,
            this, &SallesWidget::onPassageValide);
    connect(source, &DataSource::erreur,
            this, &SallesWidget::onSourceErreur);
    connect(source, &DataSource::logAppend,
            this, &SallesWidget::onSourceLog);
    connect(source, &DataSource::alerte,
            this, &SallesWidget::onAlerte);
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
    for (const Salle& salle : m_source->salles()) {
        m_history->recordSalle(salle);
        m_groupeSalle.insert(salle.id, salle.groupeId);
        if (salle.groupeId.isEmpty())
            m_grid->majSalle(salle);
    }
    for (const QString& id : m_source->groupes().keys())
        mettreAJourGroupe(id);
    actualiserListeMasquees();
}

void SallesWidget::mettreAJourGroupe(const QString& id)
{
    if (!m_source || !m_source->groupes().contains(id))
        return;
    const Groupe groupe = m_source->groupes().value(id);

    GroupeVue vue;
    vue.groupe = groupe;
    for (const Salle& salle : m_source->salles()) {
        if (salle.groupeId != id)
            continue;
        ++vue.nbPortes;
        if (salle.enLigne && !salle.enAttente)
            ++vue.nbEnLigne;
        if (salle.occupation >= 0) {
            vue.occupation += salle.occupation;
            vue.capacite += salle.capacite;
        }
        if (salle.decisionFlux == QStringLiteral("redirection")
            && !salle.redirectionVers.isEmpty()) {
            if (!vue.redirectionTexte.isEmpty())
                vue.redirectionTexte += QStringLiteral("   |   ");
            vue.redirectionTexte += QStringLiteral("%1 → %2").arg(salle.id, salle.redirectionVers);
        }
    }

    if (vue.nbPortes == 0 || vue.nbEnLigne == 0) {
        vue.statut = vue.nbPortes == 0 ? QStringLiteral("ok") : QStringLiteral("offline");
    } else if (vue.capacite > 0 && double(vue.occupation) / double(vue.capacite) >= 0.95) {
        vue.statut = QStringLiteral("sature");
    } else if (vue.capacite > 0 && double(vue.occupation) / double(vue.capacite) >= 0.80) {
        vue.statut = QStringLiteral("attention");
    } else {
        vue.statut = QStringLiteral("ok");
    }

    m_grid->majGroupe(vue);
}

void SallesWidget::ouvrirStade(const QString& id)
{
    if (!m_source || !m_source->groupes().contains(id))
        return;

    if (m_stadeDialog && m_stadeDialog->objectName() == id) {
        m_stadeDialog->raise();
        m_stadeDialog->activateWindow();
        return;
    }
    if (m_stadeDialog) {
        m_stadeDialog->close();
        m_stadeDialog = nullptr;
    }

    auto* stade = new StadeWidget(m_source, id, this);
    stade->setObjectName(id);
    stade->setWindowTitle(QStringLiteral("Stade — %1").arg(id));
    stade->resize(1040, 620);
    m_stadeDialog = stade;

    connect(stade, &StadeWidget::detailPorteDemande, this,
            [this](const QString& salleId) {
                if (m_source && m_source->salles().contains(salleId))
                    onCourbeDemandee(m_source->salles().value(salleId));
            });

    stade->show();
}

void SallesWidget::confirmerSuppressionSalle(const QString& id)
{
    if (id.isEmpty() || !m_source || !m_source->salles().contains(id))
        return;

    const Salle salle = m_source->salles().value(id);
    const QString libelle = salle.nom.isEmpty() ? id
                                                : QStringLiteral("%1 (%2)")
                                                      .arg(salle.nom, id);
    const auto reponse = QMessageBox::question(
        this, QStringLiteral("Supprimer la porte"),
        QStringLiteral("Supprimer définitivement la porte %1 ?\n"
                       "La carte sera retirée et les données locales "
                       "de la salle seront effacées.")
            .arg(libelle),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reponse != QMessageBox::Yes)
        return;

    m_source->supprimerSalle(id);
}

void SallesWidget::actualiserListeMasquees()
{
    if (m_source)
        m_config->setSallesMasquees(m_source->salles(), m_grid->sallesMasquees());
}
