#pragma once

#include <QDateTimeEdit>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QWidget>

#include "data/Salle.h"

class ConfigurationTab : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigurationTab(QWidget* parent = nullptr);

    void afficherSalle(const Salle& s);
    Salle configActuelle() const;
    void configConfirmee(const QString& detail, int latenceMs);
    void testRetour(const QString& composant, bool ok, int latenceMs);

signals:
    void configDemandee(const Salle& cfg);
    void testDemande(const QString& salleId, const QString& composant,
                     const QString& valeur);
    void evacuationForcee(const QString& salleId, bool actif);
    void resetAlertes(const QString& salleId);

private:
    QLineEdit* m_nom = nullptr;
    QSpinBox* m_capacite = nullptr;
    QSpinBox* m_seuilEvac = nullptr;
    QDateTimeEdit* m_debut = nullptr;
    QDateTimeEdit* m_fin = nullptr;
    QLabel* m_retour = nullptr;
    QString m_salleId;
};
