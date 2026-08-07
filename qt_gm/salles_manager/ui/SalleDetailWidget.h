#pragma once

#include <QWidget>

#include "models/Salle.h"

class DataSource;
class IntegratedPlotWidget;
class QLabel;

class SalleDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SalleDetailWidget(DataSource* source, const QString& salleId,
                               QWidget* parent = nullptr);

private slots:
    void onSalleMiseAJour(const QString& id);

private:
    void afficher(const Salle& salle);
    double debitInstantane(const Salle& salle) const;

    DataSource* m_source = nullptr;
    QString m_salleId;
    QLabel* m_titre = nullptr;
    QLabel* m_statut = nullptr;
    QLabel* m_occupation = nullptr;
    QLabel* m_taux = nullptr;
    QLabel* m_debit = nullptr;
    QLabel* m_entrees = nullptr;
    QLabel* m_sorties = nullptr;
    QLabel* m_infos = nullptr;
    IntegratedPlotWidget* m_plot = nullptr;
};
