#pragma once

#include <QHash>
#include <QStringList>
#include <QWidget>

#include "models/Salle.h"

class QComboBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class SalleConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SalleConfigWidget(QWidget* parent = nullptr);

    void afficherCreation();
    void afficherSalle(const Salle& salle);
    void afficherMesure(const QString& id, double centimetres,
                        bool succes, const QString& note);
    void afficherErreur(const QString& message);
    void afficherInfo(const QString& message);
    void setSallesMasquees(const QHash<QString, Salle>& salles,
                           const QStringList& idsMasques);

signals:
    void creerDemandee(const Salle& salle);
    void modificationDemandee(const Salle& salle);
    void mesureDemandee(const QString& id);
    void actualisationDemandee(const QString& id);
    void masquageDemande(const QString& id);
    void courbeDemandee(const Salle& salle);
    void nouvelleDemandee();
    void restaurationDemandee(const QString& id);

private:
    Salle lireFormulaire() const;
    void actualiserEtatBoutons();
    void setModeCreation(bool creation);

    QLineEdit* m_id = nullptr;
    QLineEdit* m_nom = nullptr;
    QSpinBox* m_capacite = nullptr;
    QSpinBox* m_seuilEvacuation = nullptr;
    QDateTimeEdit* m_debut = nullptr;
    QDateTimeEdit* m_fin = nullptr;
    QLineEdit* m_hauteur = nullptr;
    QPushButton* m_boutonMesure = nullptr;
    QPushButton* m_boutonPrincipal = nullptr;
    QPushButton* m_boutonNouveau = nullptr;
    QPushButton* m_boutonActualiser = nullptr;
    QPushButton* m_boutonMasquer = nullptr;
    QPushButton* m_boutonCourbe = nullptr;
    QComboBox* m_sallesMasquees = nullptr;
    QPushButton* m_boutonRestaurer = nullptr;
    QLabel* m_titre = nullptr;
    QLabel* m_retour = nullptr;

    QString m_salleId;
    double m_hauteurCm = -1.0;
    bool m_hauteurMesuree = false;
    bool m_modeCreation = true;
};
