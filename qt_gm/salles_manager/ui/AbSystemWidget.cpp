#include "AbSystemWidget.h"

#include <QDateTime>
#include <QFont>
#include <QPainter>
#include <QPen>
#include <QPainterPath>

namespace {
const QColor cFond(0xFF, 0xFF, 0xFF);
const QColor cBordure(0xE2, 0xE8, 0xF0);
const QColor cTexte(0x1E, 0x29, 0x3B);
const QColor cTexteSecondaire(0x64, 0x74, 0x8B);
const QColor cMur(0xE2, 0xE8, 0xF0);
const QColor cMurBord(0x94, 0xA3, 0xB8);
const QColor cPassage(0xEF, 0xF6, 0xFF);
const QColor cPassageBord(0xBF, 0xDB, 0xFE);
const QColor cInactif(0xCB, 0xD5, 0xE1);
const QColor cVert(0x10, 0xB9, 0x81);
const QColor cOrange(0xF5, 0x9E, 0x0B);
const QColor cGrisBadge(0x94, 0xA3, 0xB8);
}

AbSystemWidget::AbSystemWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(190);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_timerAnimation.setInterval(350);
    connect(&m_timerAnimation, &QTimer::timeout, this, [this]() {
        if (m_phaseAnimation == 1) {
            m_etat = m_derniereDirection == QStringLiteral("entree")
                         ? QStringLiteral("vu_b")
                         : QStringLiteral("vu_a");
            m_phaseAnimation = 2;
            m_timerAnimation.start();
        } else {
            m_etat = QStringLiteral("attente");
            m_phaseAnimation = 0;
            m_timerAnimation.stop();
        }
        update();
    });
}

void AbSystemWidget::setEtat(const QString& etat, qint64)
{
    m_timerAnimation.stop();
    m_phaseAnimation = 0;
    if (m_etat == etat)
        return;
    m_etat = etat;
    update();
}

void AbSystemWidget::setDernierPassage(const QString& direction, qint64 timestampMs)
{
    m_derniereDirection = direction;
    m_dernierPassageMs = timestampMs;
    lancerAnimation();
    update();
}

void AbSystemWidget::lancerAnimation()
{
    // Séquence visualisée : A puis B (entrée), B puis A (sortie).
    m_etat = m_derniereDirection == QStringLiteral("entree")
                 ? QStringLiteral("vu_a")
                 : QStringLiteral("vu_b");
    m_phaseAnimation = 1;
    m_timerAnimation.start();
}

QSize AbSystemWidget::sizeHint() const
{
    return QSize(700, 190);
}

void AbSystemWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int W = width();
    const int H = height();

    // Carte blanche arrondie
    QPainterPath fond;
    fond.addRoundedRect(QRectF(0, 0, W, H), 10, 10);
    painter.fillPath(fond, cFond);
    painter.setPen(QPen(cBordure, 1));
    painter.drawPath(fond);

    QFont titreFont = font();
    titreFont.setBold(true);
    titreFont.setPointSize(10);
    painter.setFont(titreFont);
    painter.setPen(cTexte);
    painter.drawText(QRect(14, 10, W - 180, 22), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("SYSTÈME A-B — VALIDATION DIRECTIONNELLE"));

    // Badge d'état
    QString badgeTexte;
    QColor badgeCouleur = cGrisBadge;
    const bool animEntree = m_phaseAnimation > 0
                            && m_derniereDirection == QStringLiteral("entree");
    const bool animSortie = m_phaseAnimation > 0
                            && m_derniereDirection == QStringLiteral("sortie");
    if (animEntree) {
        badgeTexte = m_phaseAnimation == 1
                         ? QStringLiteral("A VU — ENTRÉE EN COURS")
                         : QStringLiteral("B VU — ENTRÉE VALIDÉE (A→B)");
        badgeCouleur = cVert;
    } else if (animSortie) {
        badgeTexte = m_phaseAnimation == 1
                         ? QStringLiteral("B VU — SORTIE EN COURS")
                         : QStringLiteral("A VU — SORTIE VALIDÉE (B→A)");
        badgeCouleur = cOrange;
    } else if (m_etat == QStringLiteral("vu_a")) {
        badgeTexte = QStringLiteral("A VU — ENTRÉE EN COURS");
        badgeCouleur = cVert;
    } else if (m_etat == QStringLiteral("vu_b")) {
        badgeTexte = QStringLiteral("B VU — SORTIE EN COURS");
        badgeCouleur = cOrange;
    } else {
        badgeTexte = QStringLiteral("ATTENTE D'UN PASSAGE");
    }

    QFont badgeFont = font();
    badgeFont.setBold(true);
    badgeFont.setPointSize(8);
    painter.setFont(badgeFont);
    const int badgeW = painter.fontMetrics().horizontalAdvance(badgeTexte) + 22;
    const int badgeH = 22;
    const QRect badgeRect(W - 14 - badgeW, 10, badgeW, badgeH);
    painter.setPen(Qt::NoPen);
    painter.setBrush(badgeCouleur);
    painter.drawRoundedRect(badgeRect, 6, 6);
    painter.setPen(Qt::white);
    painter.drawText(badgeRect, Qt::AlignCenter, badgeTexte);

    // --- Schéma : vue de dessus de la porte ---
    const int schemaTop = 44;
    const int schemaBottom = H - 44;
    const int midY = (schemaTop + schemaBottom) / 2;
    const int x0 = W * 0.30;
    const int x1 = W * 0.70;

    const bool aVu = m_etat == QStringLiteral("vu_a");
    const bool bVu = m_etat == QStringLiteral("vu_b");

    // Murs (blocs de part et d'autre du passage)
    painter.setPen(QPen(cMurBord, 1));
    painter.setBrush(cMur);
    painter.drawRoundedRect(QRect(x0 - 58, midY - 52, 44, 104), 4, 4);
    painter.drawRoundedRect(QRect(x1 + 14, midY - 52, 44, 104), 4, 4);

    // Passage (bande centrale)
    painter.setPen(QPen(cPassageBord, 1, Qt::DashLine));
    painter.setBrush(cPassage);
    painter.drawRect(QRect(x0, midY - 15, x1 - x0, 30));

    // Libellés EXTÉRIEUR / INTÉRIEUR
    QFont smallFont = font();
    smallFont.setPointSize(8);
    smallFont.setBold(true);
    painter.setFont(smallFont);
    painter.setPen(cTexteSecondaire);
    painter.drawText(QRect(14, midY - 10, x0 - 78, 20),
                     Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("EXTÉRIEUR"));
    painter.drawText(QRect(x1 + 68, midY - 10, W - x1 - 68 - 14, 20),
                     Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("INTÉRIEUR"));

    // Flèche directionnelle dans le passage
    const int ax0 = x0 + 34;
    const int ax1 = x1 - 34;
    const bool dim = m_phaseAnimation == 0 && !aVu && !bVu;
    if (dim) {
        painter.setPen(QPen(cInactif, 2, Qt::DashLine));
        painter.drawLine(ax0, midY, ax1, midY);
        painter.setBrush(cInactif);
        QPolygon teteG(3);
        teteG.setPoints(3, ax0, midY, ax0 + 9, midY - 5, ax0 + 9, midY + 5);
        painter.drawPolygon(teteG);
        QPolygon teteD(3);
        teteD.setPoints(3, ax1, midY, ax1 - 9, midY - 5, ax1 - 9, midY + 5);
        painter.drawPolygon(teteD);
    } else {
        const bool versInterieur = animEntree || (!animSortie && aVu);
        const QColor couleur = versInterieur ? cVert : cOrange;
        painter.setPen(QPen(couleur, 2));
        painter.drawLine(ax0, midY, ax1, midY);
        painter.setBrush(couleur);
        QPolygon tete(3);
        if (versInterieur) {
            tete.setPoints(3, ax1, midY, ax1 - 9, midY - 5, ax1 - 9, midY + 5);
        } else {
            tete.setPoints(3, ax0, midY, ax0 + 9, midY - 5, ax0 + 9, midY + 5);
        }
        painter.drawPolygon(tete);
    }

    // Capteurs A et B
    const int rayon = 11;
    const QPoint posA(x0 + 15, midY);
    const QPoint posB(x1 - 15, midY);

    auto dessinerCapteur = [&](const QPoint& centre, const QColor& couleur,
                               bool actif, const QString& nom) {
        if (actif) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(couleur.red(), couleur.green(), couleur.blue(), 60));
            painter.drawEllipse(centre, rayon + 7, rayon + 7);
        }
        painter.setPen(QPen(couleur, actif ? 2 : 1));
        painter.setBrush(actif ? couleur : cFond);
        painter.drawEllipse(centre, rayon, rayon);
        painter.setFont(smallFont);
        painter.setPen(cTexteSecondaire);
        painter.drawText(QRect(centre.x() - 90, midY + 16, 180, 18),
                         Qt::AlignCenter, nom);
    };
    dessinerCapteur(posA, cVert, aVu,
                    QStringLiteral("A · VL53L0X (ToF) — devant"));
    dessinerCapteur(posB, cOrange, bVu,
                    QStringLiteral("B · HC-SR04 (ultrason) — derrière"));

    // Bas : dernier passage validé + rappel de la logique
    painter.setFont(smallFont);
    QString derniereLigne;
    QColor derniereCouleur = cTexteSecondaire;
    if (m_derniereDirection == QStringLiteral("entree")) {
        derniereLigne = QStringLiteral("Dernier passage : ENTRÉE (A→B)  ")
                        + QDateTime::fromMSecsSinceEpoch(m_dernierPassageMs)
                              .toString(QStringLiteral("HH:mm:ss"));
        derniereCouleur = cVert;
    } else if (m_derniereDirection == QStringLiteral("sortie")) {
        derniereLigne = QStringLiteral("Dernier passage : SORTIE (B→A)  ")
                        + QDateTime::fromMSecsSinceEpoch(m_dernierPassageMs)
                              .toString(QStringLiteral("HH:mm:ss"));
        derniereCouleur = cOrange;
    } else {
        derniereLigne = QStringLiteral("Aucun passage validé");
    }
    painter.setPen(derniereCouleur);
    painter.drawText(QRect(14, H - 28, W / 2 - 20, 18),
                     Qt::AlignLeft | Qt::AlignVCenter, derniereLigne);

    painter.setPen(cTexteSecondaire);
    painter.drawText(QRect(W / 2, H - 28, W / 2 - 28, 18),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QStringLiteral("A puis B = entrée · B puis A = sortie"));
}