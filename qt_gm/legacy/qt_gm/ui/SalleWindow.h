#pragma once

#include <QWidget>

#include "models/SalleGm.h"

class QCheckBox;
class QLabel;
class QPushButton;
class GmSource;
class PlotGm;

class SalleWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SalleWindow(GmSource* source, const QString& salleId,
                         QWidget* parent = nullptr);

    QString salleId() const { return m_salleId; }

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onSalleMiseAJour(const QString& id);

private:
    void afficher();
    double calculerDebitInstantane() const;

    GmSource* m_source = nullptr;
    QString m_salleId;
    SalleGm m_derniere;

    QLabel* m_titre = nullptr;
    QLabel* m_dot = nullptr;
    QLabel* m_statutTexte = nullptr;

    // Box 1: Occupation
    QLabel* m_occValeur = nullptr;
    QLabel* m_occCapacite = nullptr;
    QLabel* m_occPct = nullptr;

    // Box 2: Débit instantané
    QLabel* m_debitValeur = nullptr;
    QLabel* m_debitTendance = nullptr;

    // Box 3: Entrées
    QLabel* m_entValeur = nullptr;

    // Box 4: Sorties
    QLabel* m_sortValeur = nullptr;

    // Box 5: Informations salle
    QLabel* m_infoCapacite = nullptr;
    QLabel* m_infoHoraires = nullptr;
    QLabel* m_infoDensite = nullptr;

    // Contrôles graphique
    QCheckBox* m_chkOcc = nullptr;
    QCheckBox* m_chkEnt = nullptr;
    QCheckBox* m_chkSort = nullptr;
    QPushButton* m_btnPause = nullptr;

    PlotGm* m_plot = nullptr;
};
