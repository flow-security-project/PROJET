#include "BarreSeuil.h"

#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPaintEvent>

BarreSeuil::BarreSeuil(const QString& libelle, double valeur,
                       double seuil, bool ok, QWidget* parent)
    : QWidget(parent), m_libelle(libelle), m_valeur(valeur),
      m_seuil(seuil), m_ok(ok)
{
    setFixedHeight(30);
    m_label = new QLabel(this);
    m_label->setStyleSheet("color:#555555;font-size:10px;");
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(1);
    lay->addWidget(m_label);
    relabel();
}

void BarreSeuil::setValeur(double valeur, bool ok)
{
    m_valeur = valeur;
    m_ok = ok;
    relabel();
    update();
}

void BarreSeuil::setSeuil(double seuil)
{
    m_seuil = seuil;
    relabel();
    update();
}

void BarreSeuil::relabel()
{
    if (!m_label)
        return;
    m_label->setText(QString("%1 — %2 %3")
                         .arg(m_libelle)
                         .arg(m_valeur, 0, 'f', 0)
                         .arg(m_ok ? "OK" : "hors seuil"));
}

void BarreSeuil::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    const int h = height();
    const int yBarre = m_label ? m_label->height() + 2 : 12;
    const int hBarre = qMax(4, h - yBarre - 4);

    const QColor coul = m_ok ? QColor("#2E7D32") : QColor("#C62828");

    // Rail
    p.setBrush(QColor("#EEEEEE"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRect(0, yBarre, width(), hBarre), 2, 2);

    // Remplissage proportionnel au seuil (100% = seuil OK)
    const double ratio = m_seuil > 0.0 ? qBound(0.0, m_valeur / m_seuil, 1.0) : 0.0;
    if (ratio > 0.0) {
        p.setBrush(coul);
        p.drawRoundedRect(QRect(0, yBarre, int(width() * ratio), hBarre), 2, 2);
    }

    // Marqueur de seuil
    p.setPen(QPen(QColor("#1A1A1A"), 1));
    const int xSeuil = int(width() * qBound(0.02, 1.0, 1.0));
    p.drawLine(QPointF(width() - 2, yBarre), QPointF(width() - 2, yBarre + hBarre));
}
