#pragma once

#include <QLabel>
#include <QWidget>

class BarreSeuil : public QWidget
{
    Q_OBJECT

public:
    explicit BarreSeuil(const QString& libelle, double valeur,
                        double seuil, bool ok, QWidget* parent = nullptr);

    void setValeur(double valeur, bool ok);
    void setSeuil(double seuil);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void relabel();

    QString m_libelle;
    double m_valeur = 0.0;
    double m_seuil = 0.0;
    bool m_ok = true;
    QLabel* m_label = nullptr;
};
