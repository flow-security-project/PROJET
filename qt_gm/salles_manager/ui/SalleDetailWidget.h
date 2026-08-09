#pragma once

#include <QWidget>

#include "models/Salle.h"

class AlerteModel;
class DataSource;
class IntegratedPlotWidget;
class JaugeSaturation;
class QLabel;
class QListWidget;

class SalleDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SalleDetailWidget(DataSource* source, const QString& salleId,
                               AlerteModel* modele = nullptr,
                               QWidget* parent = nullptr);

private slots:
    void onSalleMiseAJour(const QString& id);

private:
    void afficher(const Salle& salle);
    void actualiserAlertes();
    double debitInstantane(const Salle& salle) const;

    DataSource* m_source = nullptr;
    AlerteModel* m_modeleAlertes = nullptr;
    QString m_salleId;
    QLabel* m_titre = nullptr;
    QLabel* m_statut = nullptr;
    QLabel* m_alerteFlux = nullptr;
    QListWidget* m_alerteListe = nullptr;
    QLabel* m_occupation = nullptr;
    QLabel* m_taux = nullptr;
    QLabel* m_debit = nullptr;
    QLabel* m_entrees = nullptr;
    QLabel* m_sorties = nullptr;
    QLabel* m_infos = nullptr;
    QLabel* m_anticipation = nullptr;
    QLabel* m_tendance = nullptr;
    QLabel* m_regime = nullptr;
    QLabel* m_confiance = nullptr;
    JaugeSaturation* m_jauge = nullptr;
    IntegratedPlotWidget* m_plot = nullptr;
};
