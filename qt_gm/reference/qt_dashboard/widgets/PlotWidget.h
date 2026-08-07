#pragma once

#include <QVector>
#include <QWidget>

class PlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlotWidget(QWidget* parent = nullptr);

    void setMaxY(double maxY) { m_maxY = maxY; update(); }
    void setTitres(const QString& titreCourbe, const QString& titreAire);
    void ajouterPoint(double occ, double dens);
    void vider();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<double> m_occ;
    QVector<double> m_dens;
    double m_maxY = 100.0;
    QString m_titreOcc = "Occupation (comptage A-B)";
    QString m_titreDens = "Densité estimée";
};
