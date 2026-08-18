#include "IntegratedPlotWidget.h"

#include <QFont>
#include <QSharedPointer>
#include <QVBoxLayout>

#include "../vendor/qcustomplot.h"

IntegratedPlotWidget::IntegratedPlotWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    m_plot = new QCustomPlot(this);
    m_plot->setMinimumSize(500, 260);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_plot);

    m_occupation = m_plot->addGraph();
    m_occupation->setName(QStringLiteral("Occupation"));
    m_occupation->setPen(QPen(QColor("#4A90D9"), 2));

    QCPAxis* densiteAxis = m_plot->axisRect(0)->addAxis(QCPAxis::atRight);
    densiteAxis->setVisible(true);
    densiteAxis->setRange(0, 1);
    densiteAxis->setLabel(QStringLiteral("Densité (0-1)"));
    densiteAxis->setLabelColor(QColor("#7C3AED"));
    densiteAxis->setTickLabelColor(QColor("#7C3AED"));
    densiteAxis->setBasePen(QPen(QColor("#7C3AED"), 1));
    densiteAxis->grid()->setVisible(false);
    m_densite = m_plot->addGraph(m_plot->xAxis, densiteAxis);
    m_densite->setName(QStringLiteral("Densité"));
    m_densite->setPen(QPen(QColor("#7C3AED"), 2, Qt::DashLine));

    m_entrees = m_plot->addGraph();
    m_entrees->setName(QStringLiteral("Entrées (cumul)"));
    m_entrees->setPen(QPen(QColor("#2E7D32"), 2));

    m_sorties = m_plot->addGraph();
    m_sorties->setName(QStringLiteral("Sorties (cumul)"));
    m_sorties->setPen(QPen(QColor("#F57C00"), 2));

    m_prevision = m_plot->addGraph();
    m_prevision->setName(QStringLiteral("Prévision"));
    QPen previsionPen(QColor("#DC2626"), 2, Qt::DashLine);
    m_prevision->setPen(previsionPen);

    m_seuil80 = new QCPItemStraightLine(m_plot);
    m_seuil80->setPen(QPen(QColor("#F57C00"), 1, Qt::DashLine));
    m_seuil80->point1->setCoords(0, 80);
    m_seuil80->point2->setCoords(1, 80);

    m_seuil100 = new QCPItemStraightLine(m_plot);
    m_seuil100->setPen(QPen(QColor("#C62828"), 1, Qt::DashLine));
    m_seuil100->point1->setCoords(0, 100);
    m_seuil100->point2->setCoords(1, 100);

    m_repSaturation = new QCPItemText(m_plot);
    m_repSaturation->setText(QStringLiteral("Saturation"));
    m_repSaturation->setColor(QColor("#DC2626"));
    m_repSaturation->setPen(QPen(QColor("#DC2626")));
    m_repSaturation->setBrush(QBrush(QColor(254, 242, 242)));
    m_repSaturation->setFont(QFont(QStringLiteral("Inter"), 9, QFont::Bold));
    m_repSaturation->setPadding(QMargins(4, 2, 4, 2));
    m_repSaturation->position->setType(QCPItemPosition::ptPlotCoords);
    m_repSaturation->setVisible(false);

    m_plot->setBackground(QBrush(Qt::transparent));
    m_plot->axisRect()->setBackground(QBrush(Qt::transparent));
    m_plot->xAxis->setBasePen(QPen(QColor("#555555"), 1));
    m_plot->yAxis->setBasePen(QPen(QColor("#555555"), 1));
    m_plot->xAxis->setTickLabelColor(QColor("#555555"));
    m_plot->yAxis->setTickLabelColor(QColor("#555555"));
    m_plot->xAxis->setLabelColor(QColor("#555555"));
    m_plot->yAxis->setLabelColor(QColor("#555555"));
    m_plot->xAxis->setLabel(QStringLiteral("Heure"));
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
    auto ticker = QSharedPointer<QCPAxisTickerDateTime>::create();
    ticker->setDateTimeFormat(QStringLiteral("dd/MM HH:mm"));
    ticker->setDateTimeSpec(Qt::LocalTime);
    m_plot->xAxis->setTicker(ticker);
    m_plot->yAxis->setRange(0, 105);
    m_plot->legend->setVisible(true);
    m_plot->legend->setFont(QFont(font.family(), 9));
    m_plot->replot();
}

void IntegratedPlotWidget::setSeries(const QVector<double>& occupation,
                                     const QVector<double>& densite,
                                     const QVector<double>& entrees,
                                     const QVector<double>& sorties,
                                     int capacite,
                                     const QVector<qint64>& timestamps)
{
    m_occupation->data()->clear();
    m_densite->data()->clear();
    m_entrees->data()->clear();
    m_sorties->data()->clear();

    const double maxOccupation = double(qMax(1, capacite));
    double maxFlux = 1.0;
    for (double value : entrees)
        maxFlux = qMax(maxFlux, value);
    for (double value : sorties)
        maxFlux = qMax(maxFlux, value);

    const int count = qMin(qMin(qMin(occupation.size(), densite.size()),
                                entrees.size()),
                           sorties.size());
    m_timeBased = timestamps.size() == count && count > 0;
    for (int index = 0; index < count; ++index) {
        const double x = m_timeBased ? double(timestamps.at(index)) / 1000.0
                                     : double(index);
        m_occupation->addData(x,
                              qBound(0.0, occupation.at(index) / maxOccupation * 100.0,
                                     100.0));
        m_densite->addData(x, qBound(0.0, densite.at(index), 1.0));
        m_entrees->addData(x,
                           qBound(0.0, entrees.at(index) / maxFlux * 100.0, 100.0));
        m_sorties->addData(x,
                           qBound(0.0, sorties.at(index) / maxFlux * 100.0, 100.0));
    }

    if (m_timeBased) {
        const double last = double(timestamps.last()) / 1000.0;
        const double first = double(timestamps.first()) / 1000.0;
        m_plot->xAxis->setRange(qMax(first, last - double(m_fenetre)),
                                qMax(first + double(m_fenetre), last));
    }

    defiler();
}

void IntegratedPlotWidget::setHistoricalSeries(const QVector<HistorySample>& samples,
                                               int capacite)
{
    m_timeBased = true;
    m_occupation->data()->clear();
    m_densite->data()->clear();
    m_entrees->data()->clear();
    m_sorties->data()->clear();

    double maxFlux = 1.0;
    for (const HistorySample& sample : samples) {
        maxFlux = qMax(maxFlux, double(sample.entrees));
        maxFlux = qMax(maxFlux, double(sample.sorties));
    }
    const double maxOccupation = double(qMax(1, capacite));
    for (const HistorySample& sample : samples) {
        const double x = double(sample.timestampMs) / 1000.0;
        const double occupation = sample.occupationValid
                                      ? double(sample.occupation)
                                      : 0.0;
        m_occupation->addData(x, qBound(0.0, occupation / maxOccupation * 100.0,
                                        100.0));
        m_densite->addData(x, sample.densiteValid
                                  ? qBound(0.0, sample.densite, 1.0)
                                  : 0.0);
        m_entrees->addData(x, qBound(0.0, double(sample.entrees) / maxFlux * 100.0,
                                    100.0));
        m_sorties->addData(x, qBound(0.0, double(sample.sorties) / maxFlux * 100.0,
                                    100.0));
    }

    if (!samples.isEmpty()) {
        const double end = double(samples.last().timestampMs) / 1000.0;
        const double start = double(samples.first().timestampMs) / 1000.0;
        const double span = qMax(60.0, end - start);
        m_plot->xAxis->setRange(start, start + span);
    }
    m_plot->replot();
}

void IntegratedPlotWidget::setPrevision(double pentePersMin, int anticipationMin,
                                        int capacite)
{
    m_prevision->data()->clear();
    m_repSaturation->setVisible(false);

    if (pentePersMin <= 0.05 || capacite <= 0
        || m_occupation->dataCount() == 0) {
        m_plot->replot();
        return;
    }

    const int count = m_occupation->dataCount();
    const double lastX = m_occupation->data()->at(count - 1)->key;
    const double lastY = m_occupation->data()->at(count - 1)->value;

    const double pentePctParSec = (pentePersMin / 60.0)
                                  / double(capacite) * 100.0;

    const double visibleDebut = m_timeBased
                                    ? lastX - double(m_fenetre)
                                    : qMax(0.0, lastX - double(m_fenetre));
    const double visibleFin = lastX + double(m_fenetre);

    double finX = lastX + double(qMax(0, anticipationMin)) * 60.0;
    finX = qMin(finX, visibleFin);
    double finY = qBound(0.0, lastY + pentePctParSec * (finX - lastX), 100.0);

    m_prevision->addData(lastX, lastY);
    m_prevision->addData(finX, finY);

    if (anticipationMin >= 0 && finY >= 99.0) {
        m_repSaturation->position->setCoords(finX, 101.0);
        m_repSaturation->setText(anticipationMin == 0
                                     ? QStringLiteral("Saturée")
                                     : QStringLiteral("Saturation dans %1 min")
                                           .arg(anticipationMin));
        m_repSaturation->setVisible(true);
    }

    m_plot->xAxis->setRange(visibleDebut, qMax(visibleFin, finX));
    m_plot->replot();
}

void IntegratedPlotWidget::setGraphVisible(int index, bool visible)
{
    if (index == 0)
        m_occupation->setVisible(visible);
    else if (index == 1)
        m_entrees->setVisible(visible);
    else if (index == 2)
        m_sorties->setVisible(visible);
    else if (index == 3)
        m_densite->setVisible(visible);
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
    if (count == 0)
        return;
    const double last = m_occupation->data()->at(count - 1)->key;
    if (m_timeBased) {
        const double first = m_occupation->data()->at(0)->key;
        m_plot->xAxis->setRange(qMax(first, last - double(m_fenetre)),
                                qMax(first + double(m_fenetre), last));
    } else {
        const double start = qMax(0.0, last - double(m_fenetre));
        m_plot->xAxis->setRange(start, qMax(double(m_fenetre), start + m_fenetre));
    }
    m_plot->replot();
}
