#pragma once

#include <QVector>
#include <QWidget>

class QCPGraph;
class QCPItemStraightLine;
class QCustomPlot;

class IntegratedPlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IntegratedPlotWidget(QWidget* parent = nullptr);

    void setSeries(const QVector<double>& occupation,
                   const QVector<double>& entrees,
                   const QVector<double>& sorties,
                   int capacite);
    void setGraphVisible(int index, bool visible);
    void setPause(bool pause);
    bool isPaused() const { return m_paused; }

private:
    void defiler();

    QCustomPlot* m_plot = nullptr;
    QCPGraph* m_occupation = nullptr;
    QCPGraph* m_entrees = nullptr;
    QCPGraph* m_sorties = nullptr;
    QCPItemStraightLine* m_seuil80 = nullptr;
    QCPItemStraightLine* m_seuil100 = nullptr;
    int m_fenetre = 120;
    bool m_paused = false;
};
