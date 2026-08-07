#include "PlotWidget.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPaintEvent>

PlotWidget::PlotWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(180);
}

void PlotWidget::setTitres(const QString& titreCourbe, const QString& titreAire)
{
    m_titreOcc = titreCourbe;
    m_titreDens = titreAire;
}

void PlotWidget::vider()
{
    m_occ.clear();
    m_dens.clear();
    update();
}

void PlotWidget::ajouterPoint(double occ, double dens)
{
    m_occ.push_back(occ);
    m_dens.push_back(dens);
    if (m_occ.size() > 300) {
        m_occ.removeFirst();
        m_dens.removeFirst();
    }
    update();
}

void PlotWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int mL = 46, mR = 12, mT = 26, mB = 24;
    const QRectF zone(mL, mT, width() - mL - mR, height() - mT - mB);
    if (zone.width() <= 0 || zone.height() <= 0)
        return;

    const int n = m_occ.size();

    // Grille pointillée
    p.setPen(QPen(QColor("#E8E8E8"), 1, Qt::DotLine));
    const int grilles = 4;
    for (int i = 0; i <= grilles; i++) {
        const double y = zone.top() + zone.height() * i / grilles;
        p.drawLine(QPointF(zone.left(), y), QPointF(zone.right(), y));
    }
    for (int i = 0; i <= 6; i++) {
        const double x = zone.left() + zone.width() * i / 6;
        p.drawLine(QPointF(x, zone.top()), QPointF(x, zone.bottom()));
    }

    // Axes
    p.setPen(QPen(QColor("#555555"), 1));
    p.drawLine(zone.bottomLeft(), zone.bottomRight());
    p.drawLine(zone.topLeft(), zone.bottomLeft());

    // Labels Y (valeurs max)
    QFont fSmall = font();
    fSmall.setPointSize(9);
    p.setFont(fSmall);
    p.setPen(QColor("#555555"));
    for (int i = 0; i <= grilles; i++) {
        const double v = m_maxY * (grilles - i) / grilles;
        p.drawText(QRectF(0, zone.top() + zone.height() * i / grilles - 8,
                          mL - 6, 16),
                   Qt::AlignRight | Qt::AlignVCenter, QString::number(v, 'f', 0));
    }
    p.drawText(QRectF(zone.left(), zone.bottom() + 4, zone.width(), 16),
               Qt::AlignCenter, "temps (1 point / s)");

    auto toY = [&](double v) {
        return zone.bottom() - v / m_maxY * zone.height();
    };

    // Aire densité (#F57C00 à 20%)
    if (n >= 2 && m_dens.size() == n) {
        QPainterPath aire;
        aire.moveTo(zone.left(), toY(qMin(m_dens.first(), m_maxY)));
        for (int i = 1; i < n; i++) {
            const double x = zone.left() + zone.width() * i / qMax(1, n - 1);
            aire.lineTo(x, toY(qMin(m_dens.at(i), m_maxY)));
        }
        for (int i = n - 1; i >= 0; i--) {
            const double x = zone.left() + zone.width() * i / qMax(1, n - 1);
            aire.lineTo(x, zone.bottom());
        }
        p.fillPath(aire, QColor(245, 124, 0, 51));
    }

    // Courbe occupation (#1565C0, 2px)
    if (n >= 2) {
        QPainterPath courbe;
        courbe.moveTo(zone.left(), toY(qMin(m_occ.first(), m_maxY)));
        for (int i = 1; i < n; i++) {
            const double x = zone.left() + zone.width() * i / qMax(1, n - 1);
            courbe.lineTo(x, toY(qMin(m_occ.at(i), m_maxY)));
        }
        p.setPen(QPen(QColor("#1565C0"), 2));
        p.drawPath(courbe);
    }

    // Légende
    p.setPen(QColor("#555555"));
    QFont fLeg = font();
    fLeg.setPointSize(9);
    p.setFont(fLeg);
    const int ly = zone.top() - 18;
    p.setPen(QPen(QColor("#1565C0"), 3));
    p.drawLine(QPointF(zone.left(), ly + 5), QPointF(zone.left() + 18, ly + 5));
    p.setPen(QColor("#555555"));
    p.drawText(QPointF(zone.left() + 22, ly + 8), m_titreOcc);
    const int lx2 = zone.left() + 22
                    + p.fontMetrics().horizontalAdvance(m_titreOcc) + 22;
    p.setPen(QPen(QColor(245, 124, 0, 180), 3));
    p.drawLine(QPointF(lx2, ly + 5), QPointF(lx2 + 18, ly + 5));
    p.setPen(QColor("#555555"));
    p.drawText(QPointF(lx2 + 22, ly + 8), m_titreDens);
}
