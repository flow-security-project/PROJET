#pragma once

#include <QWidget>

#include "models/Alerte.h"

class AlerteModel;
class HistoryManager;
class QCheckBox;
class QPushButton;
class QVBoxLayout;

class AlertePanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AlertePanelWidget(AlerteModel* modele, HistoryManager* history,
                               QWidget* parent = nullptr);

    int nbAlertes() const;

signals:
    void voirDetailAlerte(const QString& salleId, quint64 ts);

private:
    void reconstruire();
    void ajouterRangee(const Alerte& a);

    AlerteModel* m_modele = nullptr;
    HistoryManager* m_history = nullptr;
    QCheckBox* m_filtreNonAcq = nullptr;
    QPushButton* m_exporter = nullptr;
    QWidget* m_liste = nullptr;
    QVBoxLayout* m_layListe = nullptr;
};
