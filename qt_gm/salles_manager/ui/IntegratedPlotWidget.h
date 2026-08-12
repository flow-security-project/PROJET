#pragma once

#include <QVector>
#include <QWidget>

#include "history/HistoryManager.h"

class QCPGraph;
class QCPItemStraightLine;
class QCPItemText;
class QCustomPlot;

class IntegratedPlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IntegratedPlotWidget(QWidget* parent = nullptr);

    void setSeries(const QVector<double>& occupation,
                   const QVector<double>& densite,
                   const QVector<double>& entrees,
                   const QVector<double>& sorties,
                   int capacite,
                   const QVector<qint64>& timestamps = {});
    void setHistoricalSeries(const QVector<HistorySample>& samples, int capacite);
    void setPrevision(double pentePersMin, int anticipationMin, int capacite);
    void setGraphVisible(int index, bool visible);
    void setPause(bool pause);
    bool isPaused() const { return m_paused; }

private:
    void defiler();

    QCustomPlot* m_plot = nullptr;
    QCPGraph* m_occupation = nullptr;
    QCPGraph* m_densite = nullptr;
    QCPGraph* m_entrees = nullptr;
    QCPGraph* m_sorties = nullptr;
    QCPGraph* m_prevision = nullptr;
    QCPItemStraightLine* m_seuil80 = nullptr;
    QCPItemStraightLine* m_seuil100 = nullptr;
    QCPItemText* m_repSaturation = nullptr;
    int m_fenetre = 120;
    bool m_paused = false;
    bool m_timeBased = false;
};
