#include "SalleGrid.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QStyle>

#include "widgets/Couleurs.h"

SalleGrid::SalleGrid(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("salleGrid");
}

void SalleGrid::majSalle(const Salle& s)
{
    if (!m_cartes.contains(s.id))
        construireCarte(s.id, s.nom);
    m_salles.insert(s.id, s);
    mettreAJourCarte(s.id);
}

void SalleGrid::selectionner(const QString& id)
{
    m_sel = id;
    for (auto it = m_cartes.begin(); it != m_cartes.end(); ++it)
        it->widget->setProperty("selection", it.key() == id);
    for (auto it = m_cartes.begin(); it != m_cartes.end(); ++it) {
        it->widget->style()->unpolish(it->widget);
        it->widget->style()->polish(it->widget);
    }
}

void SalleGrid::construireCarte(const QString& id, const QString& nom)
{
    auto* carte = new QWidget(this);
    carte->setObjectName("carteSalle");
    carte->setFixedSize(176, 118);
    carte->setProperty("salleId", id);

    auto* led = new QLabel(carte);
    led->setObjectName("ledCercle");
    led->setFixedSize(12, 12);

    auto* titre = new QLabel(nom, carte);
    titre->setObjectName("carteTitre");
    titre->setStyleSheet("font-size:12px;font-weight:700;color:#1A1A1A;");

    auto* occ = new QLabel("--", carte);
    occ->setObjectName("carteOcc");
    occ->setStyleSheet(
        "font-size:24px;font-weight:700;color:#1A1A1A;font-family:monospace;");

    auto* regime = new QLabel("—", carte);
    regime->setObjectName("carteRegime");
    regime->setStyleSheet("font-size:10px;color:#555555;");

    auto* evac = new QLabel("", carte);
    evac->setObjectName("carteEvac");
    evac->setStyleSheet("font-size:9px;font-weight:700;color:#C62828;");
    evac->setVisible(false);

    auto* l0 = new QHBoxLayout;
    l0->addWidget(led);
    l0->addStretch();
    auto* lay = new QVBoxLayout(carte);
    lay->setContentsMargins(10, 8, 10, 8);
    lay->setSpacing(2);
    lay->addLayout(l0);
    lay->addWidget(occ);
    lay->addWidget(titre);
    lay->addWidget(regime);
    lay->addWidget(evac);

    carte->installEventFilter(this);

    Carte c;
    c.widget = carte;
    c.led = led;
    c.titre = titre;
    c.occ = occ;
    c.regime = regime;
    c.evac = evac;
    m_cartes.insert(id, c);

    auto* grid = findChild<QGridLayout*>();
    if (!grid) {
        grid = new QGridLayout(this);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(10);
    }
    const int index = m_cartes.size() - 1;
    grid->addWidget(carte, index / 3, index % 3);
}

bool SalleGrid::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress
        || event->type() == QEvent::MouseButtonDblClick) {
        if (auto* carte = qobject_cast<QWidget*>(watched)) {
            const QString id = carte->property("salleId").toString();
            if (!id.isEmpty()) {
                selectionner(id);
                emit salleSelectionnee(id);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SalleGrid::mettreAJourCarte(const QString& id)
{
    if (!m_cartes.contains(id) || !m_salles.contains(id))
        return;
    const Salle& s = m_salles[id];
    Carte& c = m_cartes[id];

    c.titre->setText(s.nom);
    if (!s.enLigne) {
        c.led->setStyleSheet(
            "background:#616161;border-radius:6px;border:1px solid #D0D0D0;");
        c.occ->setText("HORS\nLIGNE");
        c.occ->setStyleSheet(
            "font-size:11px;font-weight:600;color:#616161;");
        c.regime->setText("non fiable");
        c.widget->setProperty("enLigne", false);
    } else {
        c.led->setStyleSheet(
            QString("background:%1;border-radius:6px;")
                .arg(couleurLedHex(s.ledCouleur).name()));
        c.occ->setText(s.occTexte());
        c.occ->setStyleSheet(
            "font-size:24px;font-weight:700;color:#1A1A1A;font-family:monospace;");
        c.regime->setText(QString("Régime : %1 — %2")
                              .arg(s.regime)
                              .arg(s.confiance > 0.5 ? "fiable" : "peu fiable"));
        c.widget->setProperty("enLigne", true);
    }
    c.evac->setVisible(s.evacuationActive);
    c.evac->setText("ÉVACUATION EN COURS");

    c.widget->style()->unpolish(c.widget);
    c.widget->style()->polish(c.widget);
}
