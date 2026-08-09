#pragma once

#include <QWidget>

class JaugeSaturation : public QWidget
{
    Q_OBJECT

public:
    explicit JaugeSaturation(QWidget* parent = nullptr);

    void setValeurs(double tauxActuel, int anticipationMin, double tendance);

    QSize sizeHint() const override { return QSize(420, 74); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    double m_taux = 0.0;
    int m_anticipationMin = -1;
    double m_tendance = 0.0;
};
