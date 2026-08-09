#pragma once

#include <QWidget>

#include "models/Alerte.h"

class AlerteModel;
class QCheckBox;
class QVBoxLayout;

class AlertePanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AlertePanelWidget(AlerteModel* modele, QWidget* parent = nullptr);

    int nbAlertes() const;

signals:
    void voirDetailAlerte(const QString& salleId, quint64 ts);

private:
    void reconstruire();
    void ajouterRangee(const Alerte& a);

    AlerteModel* m_modele = nullptr;
    QCheckBox* m_filtreNonAcq = nullptr;
    QWidget* m_liste = nullptr;
    QVBoxLayout* m_layListe = nullptr;
};
