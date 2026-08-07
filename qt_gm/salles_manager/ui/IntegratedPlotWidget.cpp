#include "IntegratedPlotWidget.h"

#include <QFont>
#include <QVBoxLayout>

#include "../vendor/qcustomplot.h"

IntegratedPlotWidget::IntegratedPlotWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    m_plot = new QCustomPlot(this);
    m_plot->setMinimumHeight(260);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_plot);

    m_occupation = m_plot->addGraph();
    m_occupation->setName(QStringLiteral("Occupation"));
    m_occupation->setPen(QPen(QColor("#4A90D9"), 2));

    m_entrees = m_plot->addGraph();
    m_entrees->setName(QStringLiteral("Entrées (cumul)"));
    m_entrees->setPen(QPen(QColor("#2E7D32"), 2));

    m_sorties = m_plot->addGraph();
    m_sorties->setName(QStringLiteral("Sorties (cumul)"));
    m_sorties->setPen(QPen(QColor("#F57C00"), 2));

    m_seuil80 = new QCPItemStraightLine(m_plot);
    m_seuil80->setPen(QPen(QColor("#F57C00"), 1, Qt::DashLine));
    m_seuil80->point1->setCoords(0, 80);
    m_seuil80->point2->setCoords(1, 80);

    m_seuil100 = new QCPItemStraightLine(m_plot);
    m_seuil100->setPen(QPen(QColor("#C62828"), 1, Qt::DashLine));
    m_seuil100->point1->setCoords(0, 100);
    m_seuil100->point2->setCoords(1, 100);

    m_plot->setBackground(QBrush(Qt::transparent));
    m_plot->axisRect()->setBackground(QBrush(Qt::transparent));
    m_plot->xAxis->setBasePen(QPen(QColor("#555555"), 1));
    m_plot->yAxis->setBasePen(QPen(QColor("#555555"), 1));
    m_plot->xAxis->setTickLabelColor(QColor("#555555"));
    m_plot->yAxis->setTickLabelColor(QColor("#555555"));
    m_plot->xAxis->setLabelColor(QColor("#555555"));
    m_plot->yAxis->setLabelColor(QColor("#555555"));
    m_plot->xAxis->setLabel(QStringLiteral("Temps (secondes)"));
    m_plot->yAxis->setLabel(QStringLiteral("Échelle normalisée (%)"));
    m_plot->xAxis->grid()->setPen(QPen(QColor("#E8E8E8"), 1, Qt::DotLine));
    m_plot->yAxis->grid()->setPen(QPen(QColor("#E8E8E8"), 1, Qt::DotLine));

    QFont font = m_plot->xAxis->tickLabelFont();
    font.setPointSize(9);
    m_plot->xAxis->setTickLabelFont(font);
    m_plot->yAxis->setTickLabelFont(font);
    m_plot->xAxis->setLabelFont(font);
    m_plot->yAxis->setLabelFont(font);
    m_plot->xAxis->setRange(0, m_fenetre);
    m_plot->yAxis->setRange(0, 105);
    m_plot->legend->setVisible(true);
    m_plot->legend->setFont(QFont(font.family(), 9));
    m_plot->replot();
}

void IntegratedPlotWidget::setSeries(const QVector<double>& occupation,
                                     const QVector<double>& entrees,
                                     const QVector<double>& sorties,
                                     int capacite)
{
    m_occupation->data()->clear();
    m_entrees->data()->clear();
    m_sorties->data()->clear();

    const double maxOccupation = double(qMax(1, capacite));
    double maxFlux = 1.0;
    for (double value : entrees)
        maxFlux = qMax(maxFlux, value);
    for (double value : sorties)
        maxFlux = qMax(maxFlux, value);

    const int count = qMin(qMin(occupation.size(), entrees.size()), sorties.size());
    for (int index = 0; index < count; ++index) {
        m_occupation->addData(index,
                              qBound(0.0, occupation.at(index) / maxOccupation * 100.0,
                                     100.0));
        m_entrees->addData(index,
                           qBound(0.0, entrees.at(index) / maxFlux * 100.0, 100.0));
        m_sorties->addData(index,
                           qBound(0.0, sorties.at(index) / maxFlux * 100.0, 100.0));
    }

    defiler();
}

void IntegratedPlotWidget::setGraphVisible(int index, bool visible)
{
    if (index == 0)
        m_occupation->setVisible(visible);
    else if (index == 1)
        m_entrees->setVisible(visible);
    else if (index == 2)
        m_sorties->setVisible(visible);
    m_plot->replot();
}

void IntegratedPlotWidget::setPause(bool pause)
{
    m_paused = pause;
}

void IntegratedPlotWidget::defiler()
{
    if (m_paused)
        return;
    const int count = m_occupation->dataCount();
    const int start = qMax(0, count - m_fenetre);
    m_plot->xAxis->setRange(start, qMax(m_fenetre, start + m_fenetre));
    m_plot->replot();
}
