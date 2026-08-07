#pragma once

#include <QTabWidget>
#include <QWidget>

#include "data/Salle.h"
#include "ui/AlertesTab.h"
#include "ui/ConfigurationTab.h"
#include "ui/VisualisationTab.h"

class SalleDetail : public QWidget
{
    Q_OBJECT

public:
    explicit SalleDetail(QWidget* parent = nullptr);

    void afficherSalle(const Salle& s);
    void configConfirmee(const QString& salleId, const QString& detail,
                         int latenceMs);
    void testRetour(const QString& salleId, const QString& composant,
                    bool ok, int latenceMs);
    void ajouterAlerte(const Alerte& a);
    void modifierAlerte(const Alerte& a);
    void viderAlertes();
    void ouvrirAlertes();

signals:
    void configDemandee(const Salle& cfg);
    void testDemande(const QString& salleId, const QString& composant,
                     const QString& valeur);
    void evacuationForcee(const QString& salleId, bool actif);
    void resetAlertes(const QString& salleId);
    void alerteAcquittee(const QString& salleId, quint64 ts);

private:
    QLabel* m_titre = nullptr;
    QTabWidget* m_tabs = nullptr;
    VisualisationTab* m_visu = nullptr;
    ConfigurationTab* m_config = nullptr;
    AlertesTab* m_alertes = nullptr;
};
