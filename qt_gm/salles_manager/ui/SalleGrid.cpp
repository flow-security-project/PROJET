#include "SalleGrid.h"

#include <algorithm>
#include <utility>

#include <QColor>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QProgressBar>
#include <QResizeEvent>
#include <QStyle>
#include <QVBoxLayout>

SalleGrid::SalleGrid(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QGridLayout(this);
    m_layout->setContentsMargins(16, 16, 16, 16);
    m_layout->setHorizontalSpacing(16);
    m_layout->setVerticalSpacing(16);
    setObjectName(QStringLiteral("salleGrid"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void SalleGrid::majSalle(const Salle& salle)
{
    m_salles.insert(salle.id, salle);
    if (!m_masquees.contains(salle.id) && !m_cartes.contains(salle.id))
        construireCarte(salle.id);
    mettreAJourCarte(salle.id);
}

void SalleGrid::viderVue()
{
    for (const Carte& carte : std::as_const(m_cartes)) {
        m_layout->removeWidget(carte.widget);
        delete carte.widget;
    }
    m_cartes.clear();
    m_salles.clear();
    m_masquees.clear();
    m_selection.clear();
}

void SalleGrid::masquerSalle(const QString& id)
{
    if (!m_cartes.contains(id))
        return;
    m_masquees.insert(id);
    const Carte carte = m_cartes.take(id);
    m_layout->removeWidget(carte.widget);
    delete carte.widget;
    if (m_selection == id)
        m_selection.clear();
    reflow();
}

bool SalleGrid::restaurerSalle(const QString& id)
{
    if (!m_salles.contains(id))
        return false;
    m_masquees.remove(id);
    if (!m_cartes.contains(id))
        construireCarte(id);
    mettreAJourCarte(id);
    return m_cartes.contains(id);
}

void SalleGrid::supprimerSalle(const QString& id)
{
    m_salles.remove(id);
    m_masquees.remove(id);
    if (m_cartes.contains(id)) {
        const Carte carte = m_cartes.take(id);
        m_layout->removeWidget(carte.widget);
        delete carte.widget;
    }
    if (m_selection == id)
        m_selection.clear();
    reflow();
}

QStringList SalleGrid::sallesMasquees() const
{
    return m_masquees.values();
}

void SalleGrid::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    reflow();
}

bool SalleGrid::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        const QString id = watched->property("salleId").toString();
        if (!id.isEmpty()) {
            setSelection(id);
            emit salleSelectionnee(id);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SalleGrid::construireCarte(const QString& id)
{
    if (m_cartes.contains(id) || !m_salles.contains(id))
        return;

    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("salleCard"));
    card->setProperty("salleId", id);
    card->setMinimumSize(300, 200);
    card->setMaximumSize(440, 240);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    card->setCursor(Qt::PointingHandCursor);

    // --- En-tête : LED + nom + badge statut ---
    auto* led = new QLabel(card);
    led->setObjectName(QStringLiteral("cardLed"));

    auto* title = new QLabel(card);
    title->setObjectName(QStringLiteral("cardTitle"));

    auto* identifiant = new QLabel(card);
    identifiant->setObjectName(QStringLiteral("cardTag"));

    auto* status = new QLabel(card);
    status->setObjectName(QStringLiteral("cardStatusBadge"));

    auto* headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);
    headerLayout->addWidget(led);
    headerLayout->addWidget(title);
    headerLayout->addWidget(identifiant);
    headerLayout->addStretch();
    headerLayout->addWidget(status);

    // --- Zone centrale d'occupation ---
    auto* occupation = new QLabel(card);
    occupation->setObjectName(QStringLiteral("cardOccupancyNum"));

    auto* pourcentage = new QLabel(card);
    pourcentage->setObjectName(QStringLiteral("cardPercent"));

    auto* occLayout = new QHBoxLayout;
    occLayout->setContentsMargins(0, 8, 0, 0);
    occLayout->addWidget(occupation);
    occLayout->addStretch();
    occLayout->addWidget(pourcentage);

    // --- Barre de progression d'occupation ---
    auto* barre = new QProgressBar(card);
    barre->setObjectName(QStringLiteral("cardProgressBar"));
    barre->setTextVisible(false);
    barre->setFixedHeight(8);
    barre->setRange(0, 100);

    // --- Zone inférieure de détails (pied de carte) ---
    auto* footer = new QFrame(card);
    footer->setObjectName(QStringLiteral("cardFooter"));

    auto* flux = new QLabel(footer);
    flux->setObjectName(QStringLiteral("cardMetric"));

    auto* details = new QLabel(footer);
    details->setObjectName(QStringLiteral("cardMetric"));

    auto* hauteur = new QLabel(footer);
    hauteur->setObjectName(QStringLiteral("cardMetricSub"));

    auto* footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(12, 10, 12, 10);
    footerLayout->setSpacing(5);

    auto* infoRow1 = new QHBoxLayout;
    infoRow1->setContentsMargins(0, 0, 0, 0);
    infoRow1->addWidget(flux);
    infoRow1->addStretch();
    infoRow1->addWidget(details);

    footerLayout->addLayout(infoRow1);
    footerLayout->addWidget(hauteur);

    // --- Layout principal de la carte : liseré d'identité + corps ---
    auto* accent = new QFrame(card);
    accent->setObjectName(QStringLiteral("cardAccent"));
    accent->setFixedWidth(8);

    auto* body = new QWidget(card);
    body->setObjectName(QStringLiteral("cardBody"));

    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    layout->addLayout(headerLayout);
    layout->addLayout(occLayout);
    layout->addWidget(barre);
    layout->addWidget(footer);

    auto* rootLayout = new QHBoxLayout(card);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(accent);
    rootLayout->addWidget(body, 1);

    const QList<QWidget*> clickable = {card, accent, body, led, title, identifiant,
                                       status, occupation, pourcentage, barre,
                                       footer, flux, details, hauteur};
    for (QWidget* widget : clickable) {
        widget->setProperty("salleId", id);
        widget->installEventFilter(this);
    }

    Carte carte;
    carte.widget = card;
    carte.accent = accent;
    carte.led = led;
    carte.titre = title;
    carte.identifiant = identifiant;
    carte.statut = status;
    carte.occupation = occupation;
    carte.pourcentage = pourcentage;
    carte.barre = barre;
    carte.flux = flux;
    carte.details = details;
    carte.hauteur = hauteur;
    m_cartes.insert(id, carte);
    reflow();
}

void SalleGrid::mettreAJourCarte(const QString& id)
{
    if (!m_cartes.contains(id) || !m_salles.contains(id))
        return;

    const Salle& salle = m_salles[id];
    Carte& carte = m_cartes[id];

    QColor base;
    if (salle.enAttente) {
        base = QColor(QStringLiteral("#6366F1"));
    } else if (!salle.enLigne) {
        base = QColor(QStringLiteral("#94A3B8"));
    } else if (salle.evacuationActive) {
        base = QColor(QStringLiteral("#B91C1C"));
    } else {
        const double t = salle.taux();
        if (t >= 0.95)
            base = QColor(QStringLiteral("#DC2626"));
        else if (t >= 0.80)
            base = QColor(QStringLiteral("#EF6C00"));
        else if (t >= 0.60)
            base = QColor(QStringLiteral("#D97706"));
        else
            base = QColor(QStringLiteral("#059669"));
    }
    carte.accent->setStyleSheet(
        QStringLiteral("background:qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                       "stop:0 %1, stop:1 %2); border:none; "
                       "border-top-left-radius:8px; border-bottom-left-radius:8px;")
            .arg(base.lighter(125).name(), base.darker(115).name()));

    carte.titre->setText(salle.nom.isEmpty() ? salle.id : salle.nom);
    carte.identifiant->setText(QStringLiteral("[%1]").arg(salle.id));

    const bool online = salle.enLigne && !salle.enAttente;
    const bool critical = salle.evacuationActive || salle.taux() >= 0.95;
    const bool warning = salle.taux() >= 0.80;
    const int pct = int(salle.taux() * 100.0);

    QString statusText;
    QString level;

    if (salle.enAttente) {
        statusText = QStringLiteral("En attente");
        level = QStringLiteral("pending");
    } else if (!salle.enLigne) {
        statusText = QStringLiteral("Hors ligne");
        level = QStringLiteral("offline");
    } else if (salle.evacuationActive) {
        statusText = QStringLiteral("ÉVACUATION");
        level = QStringLiteral("critical");
    } else if (critical) {
        statusText = QStringLiteral("CRITIQUE");
        level = QStringLiteral("critical");
    } else if (warning) {
        statusText = QStringLiteral("ATTENTION");
        level = QStringLiteral("warning");
    } else {
        statusText = QStringLiteral("EN LIGNE");
        level = QStringLiteral("normal");
    }

    carte.statut->setText(statusText);
    carte.statut->setProperty("level", level);
    carte.statut->style()->unpolish(carte.statut);
    carte.statut->style()->polish(carte.statut);

    carte.led->setProperty("level", level);
    carte.led->style()->unpolish(carte.led);
    carte.led->style()->polish(carte.led);

    carte.occupation->setText(salle.occupation < 0 
        ? QStringLiteral("-- / %1").arg(salle.capacite)
        : QStringLiteral("%1 / %2").arg(salle.occupation).arg(salle.capacite));

    carte.pourcentage->setText(salle.occupation < 0 ? QStringLiteral("-- %") : QStringLiteral("%1 %").arg(pct));
    carte.pourcentage->setProperty("level", level);
    carte.pourcentage->style()->unpolish(carte.pourcentage);
    carte.pourcentage->style()->polish(carte.pourcentage);

    carte.barre->setValue(salle.occupation < 0 ? 0 : qBound(0, pct, 100));
    carte.barre->setProperty("level", level);
    carte.barre->style()->unpolish(carte.barre);
    carte.barre->style()->polish(carte.barre);

    carte.flux->setText(QStringLiteral("Entrées : %1    Sorties : %2").arg(salle.nbEntrees).arg(salle.nbSorties));
    carte.details->setText(QStringLiteral("Densité : %1").arg(salle.densite, 0, 'f', 2));

    carte.hauteur->setText(QStringLiteral("Porte : %1    |    %2 - %3")
        .arg(salle.hauteurPorteMesuree 
            ? QStringLiteral("%1 cm").arg(salle.hauteurPorteCm, 0, 'f', 1) 
            : QStringLiteral("non mesurée"))
        .arg(salle.horaireDebut, salle.horaireFin));

    carte.widget->setProperty("level", level);
    carte.widget->setProperty("selected", id == m_selection);
    carte.widget->style()->unpolish(carte.widget);
    carte.widget->style()->polish(carte.widget);
}

void SalleGrid::reflow()
{
    const int cardWidth = 310;
    const int columns = qMax(1, (width() - 20) / (cardWidth + 16));

    QStringList sortedIds = m_cartes.keys();
    std::sort(sortedIds.begin(), sortedIds.end());

    int index = 0;
    for (const QString& id : sortedIds) {
        QWidget* w = m_cartes[id].widget;
        m_layout->removeWidget(w);
        m_layout->addWidget(w, index / columns, index % columns);
        ++index;
    }
    for (int column = 0; column < columns; ++column)
        m_layout->setColumnStretch(column, 1);
}

void SalleGrid::setSelection(const QString& id)
{
    m_selection = id;
    for (const QString& cardId : m_cartes.keys())
        mettreAJourCarte(cardId);
}

void SalleGrid::selectionnerSalle(const QString& id)
{
    if (!m_cartes.contains(id))
        return;
    setSelection(id);
    emit salleSelectionnee(id);
}
