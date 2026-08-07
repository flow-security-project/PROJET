#pragma once

#include <QWidget>

#include "data/Salle.h"
#include "widgets/BarreSeuil.h"
#include "widgets/LcdMirror.h"
#include "widgets/PlotWidget.h"

class VisualisationTab : public QWidget
{
    Q_OBJECT

public:
    explicit VisualisationTab(QWidget* parent = nullptr);

    void majSalle(const Salle& s);

private:
    PlotWidget* m_plot = nullptr;
    LcdMirror* m_lcd = nullptr;
    BarreSeuil* m_audio = nullptr;
    BarreSeuil* m_therm = nullptr;
    BarreSeuil* m_surface = nullptr;
    QLabel* m_regime = nullptr;
    QLabel* m_confiance = nullptr;
    QLabel* m_anticipation = nullptr;
    QLabel* m_conditions = nullptr;
};
