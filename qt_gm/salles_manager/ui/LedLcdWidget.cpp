#include "LedLcdWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontDatabase>

namespace {
const QColor cFond(0xFF, 0xFF, 0xFF);
const QColor cBordure(0xE2, 0xE8, 0xF0);
const QColor cTexte(0x1E, 0x29, 0x3B);
const QColor cTexteSecondaire(0x64, 0x74, 0x8B);
const QColor cInactif(0xCB, 0xD5, 0xE1);

const QColor cLcdBg(0x0F, 0x17, 0x2A);       // Arrière-plan LCD sombre
const QColor cLcdTexte(0x22, 0xC5, 0x5E);    // Vert néon LCD #22C55E
const QColor cLcdBordure(0x33, 0x41, 0x55);  // Bordure LCD

QColor colorFromName(const QString& name) {
    if (name == QStringLiteral("rouge")) return QColor(0xEF, 0x44, 0x44);
    if (name == QStringLiteral("vert")) return QColor(0x10, 0xB9, 0x81);
    if (name == QStringLiteral("orange")) return QColor(0xF5, 0x9E, 0x0B);
    if (name == QStringLiteral("jaune")) return QColor(0xD9, 0x77, 0x06);
    if (name == QStringLiteral("bleu")) return QColor(0x25, 0x63, 0xEB);
    return QColor(0xCB, 0xD5, 0xE1);
}
}

LedLcdWidget::LedLcdWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void LedLcdWidget::setAffichage(const QString& ledColor, const QString& ledColorConf, const QString& ledMode,
                                const QString& l1, const QString& l2, bool enLigne, bool enAttente)
{
    m_ledCouleur = ledColor;
    m_ledCouleurConf = ledColorConf;
    m_ledMode = ledMode;
    m_lcdLigne1 = l1;
    m_lcdLigne2 = l2;
    m_enLigne = enLigne;
    m_enAttente = enAttente;
    update();
}

QSize LedLcdWidget::sizeHint() const
{
    return QSize(700, 180);
}

void LedLcdWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int W = width();
    const int H = height();

    // 1. Carte principale (fond blanc arrondi)
    QPainterPath fond;
    fond.addRoundedRect(QRectF(0, 0, W, H), 10, 10);
    painter.fillPath(fond, cFond);
    painter.setPen(QPen(cBordure, 1));
    painter.drawPath(fond);

    // Titre de la carte
    QFont titreFont = font();
    titreFont.setBold(true);
    titreFont.setPointSize(10);
    painter.setFont(titreFont);
    painter.setPen(cTexte);
    painter.drawText(QRect(14, 10, W - 28, 22), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("MATÉRIEL EMBARQUÉ — PILOTAGE LED & MIROIR LCD"));

    // Ligne de séparation sous le titre
    painter.setPen(QPen(cBordure, 1));
    painter.drawLine(14, 38, W - 14, 38);

    const int midY = H / 2 + 10;

    // Début de la zone LCD : ~38 % de la largeur pour la LED, le reste pour l'écran
    const int zoneLcdX = int(W * 0.38);

    // ==========================================
    // 2. ZONE GAUCHE : VISUALISATION DE LA LED RGB
    // ==========================================
    const int ledX = 35;
    const int ledY = midY;
    const int rLed = 14;

    const bool ledActive = m_enLigne && !m_enAttente && m_ledCouleur != QStringLiteral("eteint");
    QColor cLed = ledActive ? colorFromName(m_ledCouleur) : cInactif;

    // Halo (si la LED brille)
    if (ledActive) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(cLed.red(), cLed.green(), cLed.blue(), 50));
        painter.drawEllipse(QPoint(ledX, ledY), rLed + 9, rLed + 9);
    }

    // Corps de la LED
    painter.setPen(QPen(cLed, ledActive ? 2 : 1));
    painter.setBrush(cLed);
    painter.drawEllipse(QPoint(ledX, ledY), rLed, rLed);

    // Textes explicatifs LED
    const int textLedX = ledX + rLed + 15;
    QFont libelleFont = font();
    libelleFont.setPointSize(9);
    painter.setFont(libelleFont);

    // Ligne 1 : Nom LED
    libelleFont.setBold(true);
    painter.setFont(libelleFont);
    painter.setPen(cTexte);
    painter.drawText(QRect(textLedX, midY - 30, zoneLcdX - 20 - textLedX - 10, 18),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("LED RGB PROGRESSIVE"));

    // Ligne 2 : État demandé
    libelleFont.setBold(false);
    painter.setFont(libelleFont);
    painter.setPen(cTexteSecondaire);
    QString labelDemande = QStringLiteral("Couleur demandée : %1").arg(ledActive ? m_ledCouleur : QStringLiteral("éteinte"));
    if (ledActive && m_ledMode == QStringLiteral("stroboscope"))
        labelDemande += QStringLiteral(" (clignotant)");
    painter.drawText(QRect(textLedX, midY - 10, zoneLcdX - 20 - textLedX - 10, 16),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     labelDemande);

    // Ligne 3 : Feedback réel
    QString feedback;
    QColor coulFeedback = cTexteSecondaire;
    if (m_enAttente) {
        feedback = QStringLiteral("Statut : en attente...");
    } else if (!m_enLigne) {
        feedback = QStringLiteral("Statut : hors ligne");
    } else if (m_ledCouleur == QStringLiteral("eteint")) {
        feedback = QStringLiteral("Statut : éteinte");
    } else if (m_ledCouleurConf == m_ledCouleur) {
        feedback = QStringLiteral("Statut : synchrone (OK)");
        coulFeedback = QColor(0x10, 0xB9, 0x81); // vert
    } else {
        feedback = QStringLiteral("Statut : envoi de commande...");
        coulFeedback = QColor(0xD9, 0x77, 0x06); // ambre
    }
    painter.setPen(coulFeedback);
    painter.drawText(QRect(textLedX, midY + 8, zoneLcdX - 20 - textLedX - 10, 16),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     feedback);

    // Séparateur vertical entre gauche et droite
    painter.setPen(QPen(cBordure, 1));
    painter.drawLine(zoneLcdX - 20, 48, zoneLcdX - 20, H - 18);

    // ==========================================
    // 3. ZONE DROITE : MIROIR LCD 16x2
    // ==========================================
    const int lcdX0 = zoneLcdX;
    const int lcdY0 = midY - 32;
    const int lcdW = W - lcdX0 - 20;
    const int lcdH = 64;

    // Titre miroir LCD
    libelleFont.setBold(true);
    painter.setFont(libelleFont);
    painter.setPen(cTexte);
    painter.drawText(QRect(lcdX0, midY - 48, lcdW, 16),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("REFLET ÉCRAN LCD (16x2)"));

    // Boîtier LCD (fond sombre)
    painter.setPen(QPen(cLcdBordure, 1));
    painter.setBrush(cLcdBg);
    painter.drawRoundedRect(QRectF(lcdX0, lcdY0, lcdW, lcdH), 4, 4);

    // Textes LCD
    QFont monospaceFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monospaceFont.setPointSize(12);
    monospaceFont.setBold(true);
    monospaceFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    painter.setFont(monospaceFont);
    painter.setPen(cLcdTexte);

    // Rendu ligne 1
    QString l1 = m_lcdLigne1.left(16).leftJustified(16, ' ');
    painter.drawText(QRect(lcdX0 + 10, lcdY0 + 8, lcdW - 20, 20),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     l1);

    // Rendu ligne 2
    QString l2 = m_lcdLigne2.left(16).leftJustified(16, ' ');
    painter.drawText(QRect(lcdX0 + 10, lcdY0 + 32, lcdW - 20, 20),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     l2);
}
