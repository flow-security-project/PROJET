#include "PlotGm.h"

#include <QFont>
#include <QVBoxLayout>

#include "vendor/qcustomplot.h"

PlotGm::PlotGm(QWidget* parent)
    : QWidget(parent)
{
    m_plot = new QCustomPlot(this);
    m_plot->setMinimumHeight(240);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_plot);

    // Une seule échelle normalisée (0–100 %) pour toutes les séries.
    m_gOcc = m_plot->addGraph();
    m_gOcc->setName("Occupation");
    m_gOcc->setPen(QPen(QColor("#4A90D9"), 2));
    m_gOcc->setLineStyle(QCPGraph::lsLine);

    m_gEnt = m_plot->addGraph();
    m_gEnt->setName("Entrées (cumul)");
    m_gEnt->setPen(QPen(QColor("#2E7D32"), 2));
    m_gEnt->setLineStyle(QCPGraph::lsLine);

    m_gSort = m_plot->addGraph();
    m_gSort->setName("Sorties (cumul)");
    m_gSort->setPen(QPen(QColor("#F57C00"), 2));
    m_gSort->setLineStyle(QCPGraph::lsLine);

    // Seuil d'alerte (80%)
    m_seuil80 = new QCPItemStraightLine(m_plot);
    m_seuil80->setPen(QPen(QColor("#F57C00"), 1, Qt::DashLine));
    m_seuil80->point1->setType(QCPItemPosition::ptPlotCoords);
    m_seuil80->point2->setType(QCPItemPosition::ptPlotCoords);
    m_seuil80->point1->setCoords(0, 80);
    m_seuil80->point2->setCoords(1, 80);

    m_txtSeuil80 = new QCPItemText(m_plot);
    m_txtSeuil80->setPositionAlignment(Qt::AlignRight | Qt::AlignBottom);
    m_txtSeuil80->position->setType(QCPItemPosition::ptAxisRectRatio);
    m_txtSeuil80->position->setCoords(0.98, 0.20); // 80% depuis le bas
    m_txtSeuil80->setText("SEUIL 80%");
    m_txtSeuil80->setFont(QFont(font().family(), 8, QFont::Bold));
    m_txtSeuil80->setColor(QColor("#F57C00"));

    // Seuil capacité max (100%)
    m_seuil100 = new QCPItemStraightLine(m_plot);
    m_seuil100->setPen(QPen(QColor("#C62828"), 1, Qt::DashLine));
    m_seuil100->point1->setType(QCPItemPosition::ptPlotCoords);
    m_seuil100->point2->setType(QCPItemPosition::ptPlotCoords);
    m_seuil100->point1->setCoords(0, 100);
    m_seuil100->point2->setCoords(1, 100);

    m_txtSeuil100 = new QCPItemText(m_plot);
    m_txtSeuil100->setPositionAlignment(Qt::AlignRight | Qt::AlignBottom);
    m_txtSeuil100->position->setType(QCPItemPosition::ptAxisRectRatio);
    m_txtSeuil100->position->setCoords(0.98, 0.02); // 100% (haut)
    m_txtSeuil100->setText("CAPACITÉ 100%");
    m_txtSeuil100->setFont(QFont(font().family(), 8, QFont::Bold));
    m_txtSeuil100->setColor(QColor("#C62828"));

    // Marqueur temporel "maintenant" : ligne pointillée + étiquette.
    m_marqueur = new QCPItemStraightLine(m_plot);
    m_marqueur->setPen(QPen(QColor("#B0B0B0"), 1, Qt::DashLine));
    m_marqueur->setClipToAxisRect(true);
    m_marqueur->point1->setType(QCPItemPosition::ptPlotCoords);
    m_marqueur->point2->setType(QCPItemPosition::ptPlotCoords);
    m_marqueur->point1->setCoords(0, 0);
    m_marqueur->point2->setCoords(0, 100);

    m_marqueurTexte = new QCPItemText(m_plot);
    m_marqueurTexte->setPositionAlignment(Qt::AlignTop | Qt::AlignHCenter);
    m_marqueurTexte->position->setType(QCPItemPosition::ptPlotCoords);
    m_marqueurTexte->position->setCoords(0, 100);
    m_marqueurTexte->setText("t = 0 s");
    m_marqueurTexte->setFont(QFont(font().family(), 9));
    m_marqueurTexte->setColor(QColor("#555555"));
    m_marqueurTexte->setBrush(QColor(255, 255, 255, 230));
    m_marqueurTexte->setPen(QPen(QColor("#CCCCCC"), 1));

    // Ligne verticale et infobulle de survol
    m_hoverLine = new QCPItemStraightLine(m_plot);
    m_hoverLine->setPen(QPen(QColor("#333333"), 1, Qt::DotLine));
    m_hoverLine->setVisible(false);
    m_hoverLine->point1->setType(QCPItemPosition::ptPlotCoords);
    m_hoverLine->point2->setType(QCPItemPosition::ptPlotCoords);

    m_hoverBox = new QCPItemText(m_plot);
    m_hoverBox->setVisible(false);
    m_hoverBox->setPositionAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_hoverBox->position->setType(QCPItemPosition::ptPlotCoords);
    m_hoverBox->setFont(QFont(font().family(), 8));
    m_hoverBox->setColor(QColor("#222222"));
    m_hoverBox->setBrush(QColor(255, 255, 255, 240));
    m_hoverBox->setPen(QPen(QColor("#B0B0B0"), 1));
    m_hoverBox->setPadding(QMargins(6, 4, 6, 4));

    // Charte graphique sobre et plate.
    m_plot->setBackground(QBrush(Qt::transparent));
    m_plot->axisRect()->setBackground(QBrush(Qt::transparent));
    m_plot->xAxis->setBasePen(QPen(QColor("#555555"), 1));
    m_plot->yAxis->setBasePen(QPen(QColor("#555555"), 1));
    m_plot->xAxis->setTickPen(QPen(QColor("#555555"), 1));
    m_plot->yAxis->setTickPen(QPen(QColor("#555555"), 1));
    m_plot->xAxis->setSubTickPen(QPen(QColor("#999999"), 1));
    m_plot->yAxis->setSubTickPen(QPen(QColor("#999999"), 1));
    m_plot->xAxis->setTickLabelColor(QColor("#555555"));
    m_plot->yAxis->setTickLabelColor(QColor("#555555"));
    m_plot->xAxis->setLabelColor(QColor("#555555"));
    m_plot->yAxis->setLabelColor(QColor("#555555"));
    m_plot->xAxis->setLabel("Temps (secondes)");
    m_plot->yAxis->setLabel("Échelle normalisée (%)");

    m_plot->xAxis->grid()->setPen(QPen(QColor("#E8E8E8"), 1, Qt::DotLine));
    m_plot->yAxis->grid()->setPen(QPen(QColor("#E8E8E8"), 1, Qt::DotLine));

    QFont f = m_plot->xAxis->tickLabelFont();
    f.setPointSize(9);
    m_plot->xAxis->setTickLabelFont(f);
    m_plot->yAxis->setTickLabelFont(f);
    m_plot->xAxis->setLabelFont(f);
    m_plot->yAxis->setLabelFont(f);

    m_plot->xAxis->setRange(0, m_fenetre);
    m_plot->yAxis->setRange(0, 105);

    connect(m_plot, &QCustomPlot::mouseMove, this, &PlotGm::onMouseMove);

    m_plot->replot();
}

void PlotGm::onMouseMove(QMouseEvent* event)
{
    if (m_rawOcc.isEmpty()) {
        m_hoverLine->setVisible(false);
        m_hoverBox->setVisible(false);
        m_plot->replot();
        return;
    }

    const double xCoord = m_plot->xAxis->pixelToCoord(event->pos().x());
    const int idx = qRound(xCoord);

    if (idx >= 0 && idx < m_rawOcc.size()) {
        m_hoverLine->point1->setCoords(idx, 0);
        m_hoverLine->point2->setCoords(idx, 105);
        m_hoverLine->setVisible(true);

        const double occVal = m_rawOcc.at(idx);
        const double entVal = m_rawEnt.at(idx);
        const double sortVal = m_rawSort.at(idx);
        const int pct = m_capacite > 0 ? qRound(occVal / m_capacite * 100.0) : 0;

        m_hoverBox->position->setCoords(idx, 95);
        m_hoverBox->setText(QString("t = %1 s\nOccupation : %2 / %3 (%4 %)\nEntrées : %5 | Sorties : %6")
                                .arg(idx)
                                .arg(int(occVal))
                                .arg(m_capacite)
                                .arg(pct)
                                .arg(int(entVal))
                                .arg(int(sortVal)));
        m_hoverBox->setVisible(true);
    } else {
        m_hoverLine->setVisible(false);
        m_hoverBox->setVisible(false);
    }

    m_plot->replot();
}

void PlotGm::setGraphVisible(int index, bool visible)
{
    if (index == 0 && m_gOcc) m_gOcc->setVisible(visible);
    if (index == 1 && m_gEnt) m_gEnt->setVisible(visible);
    if (index == 2 && m_gSort) m_gSort->setVisible(visible);
    m_plot->replot();
}

void PlotGm::setPause(bool pause)
{
    m_paused = pause;
}

void PlotGm::defiler()
{
    if (m_paused)
        return;
    const int n = m_gOcc->dataCount();
    const int debut = qMax(0, n - m_fenetre);
    m_plot->xAxis->setRange(debut, debut + m_fenetre);
    m_plot->replot();
}

QVector<double> PlotGm::lisser(const QVector<double>& v, int fenetre)
{
    QVector<double> r = v;
    if (fenetre < 2 || v.size() < fenetre)
        return r;
    const int demi = fenetre / 2;
    for (int i = 0; i < v.size(); i++) {
        const int a = qMax(0, i - demi);
        const int b = qMin(v.size() - 1, i + demi);
        double s = 0.0;
        int c = 0;
        for (int j = a; j <= b; j++) {
            s += v.at(j);
            c++;
        }
        r[i] = s / c;
    }
    return r;
}

void PlotGm::setSeries(const QVector<double>& occ,
                       const QVector<double>& ent,
                       const QVector<double>& sort,
                       int capacite)
{
    m_rawOcc = occ;
    m_rawEnt = ent;
    m_rawSort = sort;
    m_capacite = capacite;

    m_gOcc->data()->clear();
    m_gEnt->data()->clear();
    m_gSort->data()->clear();

    const double maxOcc = double(qMax(1, capacite));
    double maxFlux = 1.0;
    for (double v : ent)
        maxFlux = qMax(maxFlux, v);
    for (double v : sort)
        maxFlux = qMax(maxFlux, v);

    const QVector<double> entL = lisser(ent, 7);
    const QVector<double> sortL = lisser(sort, 7);

    const int n = qMin(qMin(occ.size(), entL.size()), sortL.size());
    for (int i = 0; i < n; i++) {
        m_gOcc->addData(i, qBound(0.0, occ.at(i) / maxOcc * 100.0, 100.0));
        m_gEnt->addData(i, qBound(0.0, entL.at(i) / maxFlux * 100.0, 100.0));
        m_gSort->addData(i, qBound(0.0, sortL.at(i) / maxFlux * 100.0, 100.0));
    }

    const int count = m_gOcc->dataCount();
    if (count > 0) {
        const double x = count - 1;
        m_marqueur->point1->setCoords(x, 0);
        m_marqueur->point2->setCoords(x, 105);
        m_marqueurTexte->position->setCoords(x, 105);
        m_marqueurTexte->setText(QString("t = %1 s").arg(count - 1));
    }
    defiler();
}

void PlotGm::vider()
{
    m_rawOcc.clear();
    m_rawEnt.clear();
    m_rawSort.clear();
    m_gOcc->data()->clear();
    m_gEnt->data()->clear();
    m_gSort->data()->clear();
    m_plot->xAxis->setRange(0, m_fenetre);
    m_plot->replot();
}
