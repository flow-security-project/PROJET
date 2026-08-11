#include "StadeWidget.h"

#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStyle>
#include <QVBoxLayout>

#include "data/DataSource.h"
#include "ui/SalleConfigWidget.h"
#include "ui/SalleGrid.h"

StadeWidget::StadeWidget(DataSource* source, const QString& groupeId, QWidget* parent)
    : QDialog(parent)
    , m_source(source)
    , m_groupeId(groupeId)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("stadeRoot"));
    setStyleSheet(QStringLiteral("QDialog#stadeRoot { background: #0F172A; }"));

    m_titre = new QLabel(this);
    m_titre->setObjectName(QStringLiteral("pageTitle"));
    m_sousTitre = new QLabel(this);
    m_sousTitre->setObjectName(QStringLiteral("statusLabel"));
    m_sousTitre->setWordWrap(true);
    m_occupation = new QLabel(this);
    m_occupation->setObjectName(QStringLiteral("cardOccupancyNum"));
    m_statut = new QLabel(this);
    m_statut->setObjectName(QStringLiteral("cardStatusBadge"));
    m_redirection = new QLabel(this);
    m_redirection->setObjectName(QStringLiteral("alerteIntrusionBadge"));
    m_redirection->setWordWrap(true);
    m_redirection->hide();

    auto* header = new QHBoxLayout;
    header->setSpacing(12);
    header->addWidget(m_titre);
    header->addWidget(m_sousTitre);
    header->addStretch();
    header->addWidget(m_occupation);
    header->addWidget(m_statut);

    auto* boutonAjouter = new QPushButton(QStringLiteral("Ajouter une porte au stade"), this);
    boutonAjouter->setObjectName(QStringLiteral("btnPrimaire"));
    boutonAjouter->setCursor(Qt::PointingHandCursor);
    connect(boutonAjouter, &QPushButton::clicked, this, &StadeWidget::afficherNouvellePorte);

    auto* boutonSupprimer = new QPushButton(QStringLiteral("Supprimer le stade"), this);
    boutonSupprimer->setObjectName(QStringLiteral("btnSupprimer"));
    boutonSupprimer->setCursor(Qt::PointingHandCursor);
    connect(boutonSupprimer, &QPushButton::clicked, this, [this]() {
        if (!m_source || !m_source->groupes().contains(m_groupeId))
            return;
        const Groupe groupe = m_source->groupes().value(m_groupeId);
        int nbPortes = 0;
        for (const Salle& salle : m_source->salles()) {
            if (salle.groupeId == m_groupeId)
                ++nbPortes;
        }
        const auto reponse = QMessageBox::question(
            this, QStringLiteral("Supprimer le stade"),
            QStringLiteral("Supprimer définitivement le stade %1 « %2 » ?\n"
                           "Les %3 porte(s) membres seront également supprimées.")
                .arg(groupe.id, groupe.nom)
                .arg(nbPortes),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reponse != QMessageBox::Yes)
            return;
        m_source->supprimerGroupe(m_groupeId);
    });

    auto* actions = new QHBoxLayout;
    actions->addWidget(boutonAjouter);
    actions->addWidget(boutonSupprimer);
    actions->addStretch();

    // Grille des portes du stade (thème dédié, distinct du MULTI)
    auto* gridLabel = new QLabel(QStringLiteral("Portes du stade"), this);
    gridLabel->setObjectName(QStringLiteral("statusLabel"));
    m_grid = new SalleGrid(this);
    m_grid->setThemeStade(true);
    auto* grilleScroll = new QScrollArea(this);
    grilleScroll->setObjectName(QStringLiteral("sallesScroll"));
    grilleScroll->setFrameShape(QFrame::NoFrame);
    grilleScroll->setWidgetResizable(true);
    grilleScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    grilleScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    grilleScroll->setWidget(m_grid);
    auto* grillePane = new QWidget(this);
    auto* grilleLayout = new QVBoxLayout(grillePane);
    grilleLayout->setContentsMargins(0, 0, 0, 0);
    grilleLayout->addWidget(gridLabel);
    grilleLayout->addWidget(grilleScroll, 1);

    // Éditeur intégré : création / édition des portes directement ici
    m_config = new SalleConfigWidget(this);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(grillePane);
    splitter->addWidget(m_config);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({500, 300});

    m_vide = new QLabel(QStringLiteral("Aucune porte dans ce stade pour le moment."), this);
    m_vide->setObjectName(QStringLiteral("statusLabel"));
    m_vide->setWordWrap(true);

    auto* fermer = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(fermer, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    layout->addLayout(header);
    layout->addWidget(m_redirection);
    layout->addLayout(actions);
    layout->addWidget(splitter, 1);
    layout->addWidget(m_vide);
    layout->addWidget(fermer);

    // Clic sur une porte du stade -> édition dans le panneau intégré
    connect(m_grid, &SalleGrid::salleSelectionnee, this, [this](const QString& id) {
        if (m_source && m_source->salles().contains(id))
            m_config->afficherSalle(m_source->salles().value(id));
    });
    connect(m_config, &SalleConfigWidget::creerDemandee, this, [this](const Salle& salle) {
        if (!m_source)
            return;
        Salle s = salle;
        s.groupeId = m_groupeId;
        s.modeFlux = m_source->groupes().value(m_groupeId).mode;
        m_source->creerSalle(s);
    });
    connect(m_config, &SalleConfigWidget::modificationDemandee, this, [this](const Salle& salle) {
        if (m_source)
            m_source->modifierSalle(salle);
    });
    connect(m_config, &SalleConfigWidget::mesureDemandee, this,
            [this](const QString& id) {
                if (m_source)
                    m_source->getHauteurPorte(id);
            });
    connect(m_config, &SalleConfigWidget::actualisationDemandee, this,
            [this](const QString& id) {
                if (m_source)
                    m_source->actualiserSalle(id);
            });
    connect(m_config, &SalleConfigWidget::nouvelleDemandee, this,
            &StadeWidget::afficherNouvellePorte);
    connect(m_config, &SalleConfigWidget::masquageDemande, this, [this](const QString& id) {
        m_grid->masquerSalle(id);
        actualiserListeMasquees();
    });
    connect(m_config, &SalleConfigWidget::restaurationDemandee, this, [this](const QString& id) {
        if (m_grid->restaurerSalle(id) && m_source)
            m_config->afficherSalle(m_source->salles().value(id));
        actualiserListeMasquees();
    });
    connect(m_config, &SalleConfigWidget::suppressionDemandee, this, [this](const QString& id) {
        if (!m_source || !m_source->salles().contains(id))
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
    });
    connect(m_config, &SalleConfigWidget::courbeDemandee, this, [this](const Salle& salle) {
        emit detailPorteDemande(salle.id);
    });

    if (m_source) {
        connect(m_source, &DataSource::salleAjoutee,
                this, &StadeWidget::actualiser);
        connect(m_source, &DataSource::salleMiseAJour,
                this, &StadeWidget::actualiser);
        connect(m_source, &DataSource::salleSupprimee,
                this, &StadeWidget::actualiser);
        connect(m_source, &DataSource::groupeMiseAJour,
                this, &StadeWidget::actualiser);
        connect(m_source, &DataSource::groupeSupprime,
                this, &StadeWidget::actualiser);
        connect(m_source, &DataSource::hauteurPorteMesuree,
                m_config, &SalleConfigWidget::afficherMesure);
        connect(m_source, &DataSource::erreur,
                m_config, &SalleConfigWidget::afficherErreur);
    }

    actualiser();
}

void StadeWidget::actualiser()
{
    if (!m_source)
        return;
    const Groupe groupe = m_source->groupes().value(m_groupeId);
    if (groupe.id.isEmpty()) {
        close();
        return;
    }
    actualiserHeader(groupe);
    actualiserPortes();
    actualiserListeMasquees();
}

void StadeWidget::actualiserHeader(const Groupe& groupe)
{
    m_titre->setText(groupe.nom.isEmpty() ? groupe.id : groupe.nom);
    m_sousTitre->setText(QStringLiteral("%1 — %2")
                             .arg(groupe.id,
                                  groupe.mode == ModeFlux::Uni
                                      ? QStringLiteral("UNI-MARKET (portails interchangeables)")
                                      : QStringLiteral("MULTI-MARKET (regroupement)")));

    int occupation = 0;
    int capacite = 0;
    int nbPortes = 0;
    int nbEnLigne = 0;
    QString redirections;
    for (const Salle& salle : m_source->salles()) {
        if (salle.groupeId != groupe.id)
            continue;
        ++nbPortes;
        if (salle.enLigne)
            ++nbEnLigne;
        if (salle.occupation >= 0)
            occupation += salle.occupation;
        capacite += salle.capacite;
        if (salle.decisionFlux == QStringLiteral("redirection") && !salle.redirectionVers.isEmpty()) {
            if (!redirections.isEmpty())
                redirections += QStringLiteral("   |   ");
            redirections += QStringLiteral("%1 → %2").arg(salle.id, salle.redirectionVers);
        }
    }

    m_occupation->setText(QStringLiteral("%1 / %2 pers. — %3 portes")
                              .arg(occupation)
                              .arg(capacite)
                              .arg(nbPortes));

    const bool sature = nbPortes > 0 && nbEnLigne > 0
                        && (occupation >= int(0.95 * capacite));
    const bool attention = !sature && capacite > 0
                           && double(occupation) / double(capacite) >= 0.8;
    QString texte = nbPortes == 0
                        ? QStringLiteral("EN LIGNE")
                        : (nbEnLigne == 0 ? QStringLiteral("HORS LIGNE")
                                          : (sature ? QStringLiteral("SATURÉ")
                                                    : (attention ? QStringLiteral("SOUS TENSION")
                                                                 : QStringLiteral("EN LIGNE"))));
    QString niveau = sature ? QStringLiteral("critical")
                            : (attention ? QStringLiteral("warning")
                                         : (nbEnLigne == 0 && nbPortes > 0
                                                ? QStringLiteral("offline")
                                                : QStringLiteral("normal")));
    m_statut->setText(texte);
    m_statut->setProperty("level", niveau);
    m_statut->style()->unpolish(m_statut);
    m_statut->style()->polish(m_statut);

    if (redirections.isEmpty()) {
        m_redirection->hide();
    } else {
        m_redirection->setText(QStringLiteral("Redirections actives : %1")
                                   .arg(redirections));
        m_redirection->show();
    }
}

void StadeWidget::actualiserPortes()
{
    if (!m_source)
        return;

    QSet<QString> membres;
    for (const Salle& salle : m_source->salles()) {
        if (salle.groupeId != m_groupeId)
            continue;
        membres.insert(salle.id);
        m_grid->majSalle(salle);
    }
    for (const QString& id : m_vuesPortes) {
        if (!membres.contains(id))
            m_grid->supprimerSalle(id);
    }
    m_vuesPortes = membres;
    m_vide->setVisible(m_vuesPortes.isEmpty());
}

void StadeWidget::actualiserListeMasquees()
{
    if (m_source)
        m_config->setSallesMasquees(m_source->salles(), m_grid->sallesMasquees());
}

void StadeWidget::afficherNouvellePorte()
{
    if (!m_source)
        return;
    const Groupe groupe = m_source->groupes().value(m_groupeId);
    if (groupe.id.isEmpty())
        return;
    m_config->afficherCreationDansGroupe(groupe.id, groupe.nom, groupe.mode);
}
