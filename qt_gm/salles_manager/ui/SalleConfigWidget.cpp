#include "SalleConfigWidget.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTime>
#include <QVBoxLayout>

#include <algorithm>

SalleConfigWidget::SalleConfigWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("configRoot"));
    setAttribute(Qt::WA_StyledBackground, true);
    m_titre = new QLabel(this);
    m_titre->setObjectName(QStringLiteral("configTitle"));

    m_id = new QLineEdit(this);
    m_id->setPlaceholderText(QStringLiteral("Ex. B204"));
    m_nom = new QLineEdit(this);
    m_nom->setPlaceholderText(QStringLiteral("Nom visible de la salle"));
    m_capacite = new QSpinBox(this);
    m_capacite->setRange(1, 5000);
    m_capacite->setValue(30);
    m_seuilEvacuation = new QSpinBox(this);
    m_seuilEvacuation->setRange(1, 100);
    m_seuilEvacuation->setValue(95);
    m_seuilEvacuation->setSuffix(QStringLiteral(" %"));
    m_debut = new QDateTimeEdit(this);
    m_debut->setDisplayFormat(QStringLiteral("hh:mm"));
    m_debut->setTime(QTime(7, 0));
    m_fin = new QDateTimeEdit(this);
    m_fin->setDisplayFormat(QStringLiteral("hh:mm"));
    m_fin->setTime(QTime(22, 0));

    m_hauteur = new QLineEdit(this);
    m_hauteur->setReadOnly(true);
    m_hauteur->setPlaceholderText(QStringLiteral("Mesure obligatoire"));
    m_boutonMesure = new QPushButton(QStringLiteral("Mesurer la hauteur de porte"), this);

    auto* form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->addRow(QStringLiteral("Identifiant"), m_id);
    form->addRow(QStringLiteral("Nom"), m_nom);
    form->addRow(QStringLiteral("Capacité"), m_capacite);
    form->addRow(QStringLiteral("Seuil évacuation"), m_seuilEvacuation);
    form->addRow(QStringLiteral("Ouverture"), m_debut);
    form->addRow(QStringLiteral("Fermeture"), m_fin);
    form->addRow(QStringLiteral("Hauteur mesurée"), m_hauteur);
    form->addRow(QString(), m_boutonMesure);

    auto* configuration = new QGroupBox(QStringLiteral("Création et configuration"), this);
    configuration->setObjectName(QStringLiteral("configCard"));
    auto* configurationLayout = new QVBoxLayout(configuration);
    configurationLayout->setContentsMargins(10, 12, 10, 10);
    configurationLayout->addLayout(form);

    m_boutonPrincipal = new QPushButton(this);
    m_boutonPrincipal->setObjectName(QStringLiteral("btnPrimaire"));
    m_boutonNouveau = new QPushButton(QStringLiteral("Nouvelle salle"), this);
    m_boutonActualiser = new QPushButton(QStringLiteral("Actualiser les données"), this);
    m_boutonMasquer = new QPushButton(QStringLiteral("Masquer la carte"), this);
    m_boutonCourbe = new QPushButton(QStringLiteral("Afficher la courbe"), this);
    m_retour = new QLabel(this);
    m_retour->setWordWrap(true);
    m_retour->setObjectName(QStringLiteral("statusLabel"));

    configurationLayout->addWidget(m_boutonPrincipal);
    configurationLayout->addWidget(m_retour);

    auto* actions = new QGroupBox(QStringLiteral("Actions de la salle"), this);
    actions->setObjectName(QStringLiteral("actionCard"));
    auto* actionsLayout = new QVBoxLayout(actions);
    actionsLayout->setContentsMargins(10, 12, 10, 10);
    actionsLayout->addWidget(m_boutonNouveau);
    actionsLayout->addWidget(m_boutonActualiser);
    actionsLayout->addWidget(m_boutonCourbe);
    actionsLayout->addWidget(m_boutonMasquer);

    m_sallesMasquees = new QComboBox(this);
    m_sallesMasquees->setPlaceholderText(QStringLiteral("Aucune salle masquée"));
    m_boutonRestaurer = new QPushButton(QStringLiteral("Réafficher la carte"), this);
    auto* restauration = new QGroupBox(QStringLiteral("Cartes masquées"), this);
    restauration->setObjectName(QStringLiteral("restoreCard"));
    auto* restaurationLayout = new QVBoxLayout(restauration);
    restaurationLayout->setContentsMargins(10, 12, 10, 10);
    restaurationLayout->addWidget(m_sallesMasquees);
    restaurationLayout->addWidget(m_boutonRestaurer);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    layout->addWidget(m_titre);
    layout->addWidget(configuration);
    layout->addWidget(actions);
    layout->addWidget(restauration);
    layout->addStretch();

    connect(m_boutonMesure, &QPushButton::clicked, this, [this]() {
        const QString id = m_id->text().trimmed();
        if (id.isEmpty()) {
            afficherErreur(QStringLiteral("Saisissez un identifiant avant la mesure."));
            return;
        }
        m_boutonMesure->setEnabled(false);
        afficherInfo(QStringLiteral("Mesure en cours pour %1…").arg(id));
        emit mesureDemandee(id);
    });
    connect(m_boutonPrincipal, &QPushButton::clicked, this, [this]() {
        const Salle salle = lireFormulaire();
        if (m_modeCreation)
            emit creerDemandee(salle);
        else
            emit modificationDemandee(salle);
    });
    connect(m_boutonNouveau, &QPushButton::clicked,
            this, &SalleConfigWidget::nouvelleDemandee);
    connect(m_boutonActualiser, &QPushButton::clicked, this, [this]() {
        emit actualisationDemandee(m_salleId);
    });
    connect(m_boutonMasquer, &QPushButton::clicked, this, [this]() {
        emit masquageDemande(m_salleId);
    });
    connect(m_boutonCourbe, &QPushButton::clicked, this, [this]() {
        emit courbeDemandee(lireFormulaire());
    });
    connect(m_boutonRestaurer, &QPushButton::clicked, this, [this]() {
        const QString id = m_sallesMasquees->currentData().toString();
        if (!id.isEmpty())
            emit restaurationDemandee(id);
    });

    const QList<QWidget*> fields = {m_id, m_nom, m_capacite, m_seuilEvacuation,
                                    m_debut, m_fin};
    for (QWidget* field : fields) {
        if (auto* line = qobject_cast<QLineEdit*>(field))
            connect(line, &QLineEdit::textChanged, this,
                    &SalleConfigWidget::actualiserEtatBoutons);
        else if (auto* spin = qobject_cast<QSpinBox*>(field))
            connect(spin, &QSpinBox::valueChanged, this,
                    &SalleConfigWidget::actualiserEtatBoutons);
        else if (auto* time = qobject_cast<QDateTimeEdit*>(field))
            connect(time, &QDateTimeEdit::timeChanged, this,
                    &SalleConfigWidget::actualiserEtatBoutons);
    }

    afficherCreation();
}

void SalleConfigWidget::afficherCreation()
{
    setModeCreation(true);
    m_salleId.clear();
    m_id->clear();
    m_id->setReadOnly(false);
    m_nom->clear();
    m_capacite->setValue(30);
    m_seuilEvacuation->setValue(95);
    m_debut->setTime(QTime(7, 0));
    m_fin->setTime(QTime(22, 0));
    m_hauteurCm = -1.0;
    m_hauteurMesuree = false;
    m_hauteur->clear();
    afficherInfo(QStringLiteral("Saisissez les paramètres puis mesurez la hauteur de porte."));
}

void SalleConfigWidget::afficherSalle(const Salle& salle)
{
    setModeCreation(false);
    m_salleId = salle.id;
    m_id->setText(salle.id);
    m_nom->setText(salle.nom);
    m_capacite->setValue(salle.capacite);
    m_seuilEvacuation->setValue(salle.seuilEvacuation);
    m_debut->setTime(QTime::fromString(salle.horaireDebut, QStringLiteral("hh:mm")));
    m_fin->setTime(QTime::fromString(salle.horaireFin, QStringLiteral("hh:mm")));
    m_hauteurCm = salle.hauteurPorteCm;
    m_hauteurMesuree = salle.hauteurPorteMesuree;
    m_hauteur->setText(m_hauteurMesuree
                           ? QStringLiteral("%1 cm").arg(m_hauteurCm, 0, 'f', 1)
                           : QString());
    afficherInfo(QStringLiteral("Salle sélectionnée : %1").arg(salle.id));
    actualiserEtatBoutons();
}

void SalleConfigWidget::afficherMesure(const QString& id, double centimetres,
                                       bool succes, const QString& note)
{
    if (id != m_id->text().trimmed())
        return;
    m_boutonMesure->setEnabled(true);
    if (!succes) {
        m_hauteurCm = -1.0;
        m_hauteurMesuree = false;
        m_hauteur->clear();
        afficherErreur(note);
        actualiserEtatBoutons();
        return;
    }

    m_hauteurCm = centimetres;
    m_hauteurMesuree = true;
    m_hauteur->setText(QStringLiteral("%1 cm").arg(centimetres, 0, 'f', 1));
    afficherInfo(note);
    actualiserEtatBoutons();
}

void SalleConfigWidget::afficherErreur(const QString& message)
{
    m_retour->setObjectName(QStringLiteral("errorLabel"));
    m_retour->setText(message);
    m_retour->style()->unpolish(m_retour);
    m_retour->style()->polish(m_retour);
}

void SalleConfigWidget::afficherInfo(const QString& message)
{
    m_retour->setObjectName(QStringLiteral("statusLabel"));
    m_retour->setText(message);
    m_retour->style()->unpolish(m_retour);
    m_retour->style()->polish(m_retour);
}

void SalleConfigWidget::setSallesMasquees(const QHash<QString, Salle>& salles,
                                          const QStringList& idsMasques)
{
    const QString selectedId = m_sallesMasquees->currentData().toString();
    QStringList ids = idsMasques;
    std::sort(ids.begin(), ids.end());
    m_sallesMasquees->clear();
    for (const QString& id : ids) {
        const Salle salle = salles.value(id);
        m_sallesMasquees->addItem(salle.nom.isEmpty() ? id
                                                       : QStringLiteral("%1 (%2)")
                                                             .arg(salle.nom, id),
                                  id);
    }
    const bool hasHiddenRooms = m_sallesMasquees->count() > 0;
    if (hasHiddenRooms) {
        const int selectedIndex = m_sallesMasquees->findData(selectedId);
        m_sallesMasquees->setCurrentIndex(selectedIndex >= 0 ? selectedIndex : 0);
    }
    m_boutonRestaurer->setEnabled(hasHiddenRooms);
}

Salle SalleConfigWidget::lireFormulaire() const
{
    Salle salle;
    salle.id = m_id->text().trimmed();
    salle.nom = m_nom->text().trimmed();
    salle.capacite = m_capacite->value();
    salle.seuilEvacuation = m_seuilEvacuation->value();
    salle.horaireDebut = m_debut->time().toString(QStringLiteral("hh:mm"));
    salle.horaireFin = m_fin->time().toString(QStringLiteral("hh:mm"));
    salle.hauteurPorteCm = m_hauteurCm;
    salle.hauteurPorteMesuree = m_hauteurMesuree;
    return salle;
}

void SalleConfigWidget::actualiserEtatBoutons()
{
    const bool idOk = !m_id->text().trimmed().isEmpty();
    const bool nomOk = !m_nom->text().trimmed().isEmpty();
    const bool horairesOk = m_debut->time() < m_fin->time();
    const bool heightOk = m_hauteurMesuree && m_hauteurCm > 0.0;
    m_boutonPrincipal->setEnabled(idOk && nomOk && horairesOk && heightOk);
}

void SalleConfigWidget::setModeCreation(bool creation)
{
    m_modeCreation = creation;
    m_titre->setText(creation ? QStringLiteral("Créer une salle")
                              : QStringLiteral("Salle sélectionnée"));
    m_boutonPrincipal->setText(creation ? QStringLiteral("Créer la salle")
                                        : QStringLiteral("Enregistrer la configuration"));
    m_id->setReadOnly(!creation);
    m_boutonNouveau->setVisible(!creation);
    m_boutonActualiser->setVisible(!creation);
    m_boutonMasquer->setVisible(!creation);
    m_boutonCourbe->setVisible(!creation);
    actualiserEtatBoutons();
}
