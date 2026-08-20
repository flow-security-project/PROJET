#include "JaugeSaturation.h"

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>

JaugeSaturation::JaugeSaturation(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
}

void JaugeSaturation::setValeurs(double tauxActuel, int anticipationMin,
                                 double tendance)
{
    m_taux = std::clamp(tauxActuel, 0.0, 1.0);
    m_anticipationMin = anticipationMin;
    m_tendance = tendance;
    update();
}

void JaugeSaturation::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int widthPx = width();
    const int railY = 34;
    const int railH = 16;
    const QRect rail(0, railY, widthPx, railH);

    p.setPen(Qt::NoPen);

    // Zones de seuils : vert <60, jaune 60-80, orange 80-95, rouge ≥95
    const struct { double fin; QColor couleur; } zones[] = {
        {0.60, QColor("#10B981")},
        {0.80, QColor("#F59E0B")},
        {0.95, QColor("#F97316")},
        {1.00, QColor("#EF4444")},
    };
    double debut = 0.0;
    for (const auto& zone : zones) {
        const int x0 = int(debut * double(widthPx));
        const int x1 = int(zone.fin * double(widthPx));
        p.setBrush(zone.couleur);
        p.drawRoundedRect(QRect(x0, railY, x1 - x0 + 1, railH),
                          x0 == 0 ? 4 : 0, x0 == 0 ? 4 : 0);
        debut = zone.fin;
    }

    // Voile rouge : distance restante jusqu'à la saturation
    if (m_taux < 1.0) {
        const int x0 = int(m_taux * double(widthPx));
        const int x1 = int(0.955 * double(widthPx));
        if (x1 > x0) {
            p.setBrush(QColor(239, 68, 68, m_anticipationMin >= 0 ? 70 : 26));
            p.drawRect(QRect(x0, railY, x1 - x0 + 1, railH));
        }
    }

    // Marqueur de la position actuelle
    const int currentX = int(m_taux * double(widthPx));
    p.setPen(QPen(QColor("#F0F6FC"), 2));
    p.drawLine(QPointF(double(currentX), double(railY - 8)),
               QPointF(double(currentX), double(railY + railH + 4)));
    p.setBrush(QColor("#F0F6FC"));
    QPainterPath fleche;
    fleche.moveTo(double(currentX) - 4, double(railY - 8));
    fleche.lineTo(double(currentX) + 4, double(railY - 8));
    fleche.lineTo(double(currentX), double(railY - 14));
    fleche.closeSubpath();
    p.drawPath(fleche);

    // Libellés
    p.setPen(QColor("#8B949E"));
    QFont font(QStringLiteral("JetBrains Mono"), 9, QFont::Bold);
    p.setFont(font);
    const QString actuel = QStringLiteral("ACTUEL : %1 %")
                               .arg(int(m_taux * 100.0 + 0.5));
    p.drawText(QRect(0, 6, 180, 16), Qt::AlignLeft | Qt::AlignVCenter, actuel);

    p.drawText(QRect(widthPx - 46, railY + railH + 4, 46, 14),
               Qt::AlignRight, QStringLiteral("100 %"));

    // Message d'anticipation
    QColor messageColor;
    QString message;
    if (m_anticipationMin < 0) {
        messageColor = QColor("#059669");
        message = m_tendance > 0.05
                      ? QStringLiteral("Tendance +%1 pers/min — saturation trop lente pour être prévue")
                            .arg(m_tendance, 0, 'f', 1)
                      : QStringLiteral("Aucune saturation prévue");
    } else if (m_anticipationMin == 0) {
        messageColor = QColor("#DC2626");
        message = QStringLiteral("SALLE SATURÉE");
    } else {
        messageColor = m_anticipationMin <= 10 ? QColor("#DC2626")
                       : m_anticipationMin <= 30 ? QColor("#D97706")
                                                 : QColor("#059669");
        message = QStringLiteral("Saturation prévue dans %1 min")
                      .arg(m_anticipationMin);
    }
    font.setPointSize(11);
    font.setBold(true);
    p.setFont(font);
    p.setPen(messageColor);
    p.drawText(QRect(0, railY + railH + 4, widthPx - 50, 16),
               Qt::AlignLeft, message);
}
