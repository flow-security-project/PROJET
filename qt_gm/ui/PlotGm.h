#pragma once

#include <QWidget>

class QCPGraph;
class QCPItemStraightLine;
class QCPItemText;
class QCustomPlot;

class PlotGm : public QWidget
{
    Q_OBJECT

public:
    explicit PlotGm(QWidget* parent = nullptr);

    void setSeries(const QVector<double>& occ,
                   const QVector<double>& ent,
                   const QVector<double>& sort,
                   int capacite);
    void vider();
    void setGraphVisible(int index, bool visible);
    void setPause(bool pause);
    bool isPaused() const { return m_paused; }

private slots:
    void onMouseMove(QMouseEvent* event);

private:
    void defiler();
    static QVector<double> lisser(const QVector<double>& v, int fenetre);

    QCustomPlot* m_plot = nullptr;
    QCPGraph* m_gOcc = nullptr;
    QCPGraph* m_gEnt = nullptr;
    QCPGraph* m_gSort = nullptr;
    QCPItemStraightLine* m_marqueur = nullptr;
    QCPItemText* m_marqueurTexte = nullptr;

    // Lignes de seuil d'alerte et de capacité
    QCPItemStraightLine* m_seuil80 = nullptr;
    QCPItemText* m_txtSeuil80 = nullptr;
    QCPItemStraightLine* m_seuil100 = nullptr;
    QCPItemText* m_txtSeuil100 = nullptr;

    // Curseur d'inspection au survol
    QCPItemStraightLine* m_hoverLine = nullptr;
    QCPItemText* m_hoverBox = nullptr;

    int m_fenetre = 120;
    bool m_paused = false;

    // Conservation des séries réelles brutes pour l'infobulle
    QVector<double> m_rawOcc;
    QVector<double> m_rawEnt;
    QVector<double> m_rawSort;
    int m_capacite = 0;
};
