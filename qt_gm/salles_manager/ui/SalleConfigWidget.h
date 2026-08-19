#pragma once

#include <QHash>
#include <QStringList>
#include <QWidget>

#include "models/Groupe.h"
#include "models/Salle.h"

class QComboBox;
class QDateTimeEdit;
class QFormLayout;
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
    void afficherCreationDansGroupe(const QString& groupeId, const QString& groupeNom,
                                    ModeFlux mode);
    void afficherStatutReseau(const Salle& salle);
    void afficherMesure(const QString& id, double centimetres,
                        bool succes, const QString& note);
    void afficherErreur(const QString& message);
    void afficherInfo(const QString& message);
    void setSallesMasquees(const QHash<QString, Salle>& salles,
                           const QStringList& idsMasques);

signals:
    void creerDemandee(const Salle& salle);
    void groupeCreerDemande(const Groupe& groupe);
    void modificationDemandee(const Salle& salle);
    void mesureDemandee(const QString& id);
    void actualisationDemandee(const QString& id);
    void masquageDemande(const QString& id);
    void suppressionDemandee(const QString& id);
    void courbeDemandee(const Salle& salle);
    void nouvelleDemandee();
    void restaurationDemandee(const QString& id);

private:
    Salle lireFormulaire() const;
    Groupe lireGroupeFormulaire() const;
    void actualiserEtatBoutons();
    void setModeCreation(bool creation);
    void actualiserModeFlux();

    QComboBox* m_choixMode = nullptr;
    QComboBox* m_choixLangue = nullptr;
    QLineEdit* m_appelNumero = nullptr;
    QLineEdit* m_id = nullptr;
    QLineEdit* m_nom = nullptr;
    QSpinBox* m_capacite = nullptr;
    QSpinBox* m_seuilEvacuation = nullptr;
    QSpinBox* m_seuilEcart = nullptr;
    QDateTimeEdit* m_debut = nullptr;
    QDateTimeEdit* m_fin = nullptr;
    QLineEdit* m_hauteur = nullptr;
    QPushButton* m_boutonMesure = nullptr;
    QPushButton* m_boutonPrincipal = nullptr;
    QPushButton* m_boutonNouveau = nullptr;
    QPushButton* m_boutonActualiser = nullptr;
    QPushButton* m_boutonMasquer = nullptr;
    QPushButton* m_boutonSupprimer = nullptr;
    QPushButton* m_boutonCourbe = nullptr;
    QComboBox* m_sallesMasquees = nullptr;
    QPushButton* m_boutonRestaurer = nullptr;
    QLabel* m_titre = nullptr;
    QLabel* m_retour = nullptr;
    QLabel* m_reseauBadge = nullptr;
    QLabel* m_reseauDetail = nullptr;
    QWidget* m_reseauBox = nullptr;
    QLabel* m_groupeBadge = nullptr;
    QFormLayout* m_form = nullptr;

    QString m_salleId;
    QString m_groupeVerrouille;
    double m_hauteurCm = -1.0;
    bool m_hauteurMesuree = false;
    bool m_modeCreation = true;
};
