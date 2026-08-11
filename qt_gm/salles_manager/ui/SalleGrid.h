#pragma once

#include <QHash>
#include <QSet>
#include <QWidget>

#include "models/Groupe.h"
#include "models/Salle.h"

class QGridLayout;
class QLabel;
class QProgressBar;
class QFrame;

struct GroupeVue
{
    Groupe groupe;
    int occupation = 0;
    int capacite = 0;
    int nbPortes = 0;
    int nbEnLigne = 0;
    QString statut;           // "sature" | "attention" | "ok" | "offline"
    QString redirectionTexte; // ex. "Porte 3 → Porte 7" quand la redirection est active
};

class SalleGrid : public QWidget
{
    Q_OBJECT

public:
    explicit SalleGrid(QWidget* parent = nullptr);

    void majSalle(const Salle& salle);
    void majGroupe(const GroupeVue& vue);
    void viderVue();
    void masquerSalle(const QString& id);
    bool restaurerSalle(const QString& id);
    void supprimerSalle(const QString& id);
    void supprimerGroupe(const QString& id);
    void selectionnerSalle(const QString& id);
    QStringList sallesMasquees() const;
    void setThemeStade(bool actif) { m_themeStade = actif; }
    bool themeStade() const { return m_themeStade; }

signals:
    void salleSelectionnee(const QString& id);
    void groupeSelectionnee(const QString& id);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct Carte {
        QWidget* widget = nullptr;
        QFrame* accent = nullptr;
        QLabel* led = nullptr;
        QLabel* titre = nullptr;
        QLabel* identifiant = nullptr;
        QLabel* statut = nullptr;
        QLabel* occupation = nullptr;
        QLabel* pourcentage = nullptr;
        QProgressBar* barre = nullptr;
        QLabel* flux = nullptr;
        QLabel* details = nullptr;
        QLabel* hauteur = nullptr;
    };

    void construireCarte(const QString& id);
    void construireCarteGroupe(const QString& id);
    void mettreAJourCarte(const QString& id);
    void mettreAJourCarteGroupe(const QString& id);
    void reflow();
    void setSelection(const QString& id);

    QGridLayout* m_layout = nullptr;
    QHash<QString, Carte> m_cartes;
    QHash<QString, GroupeVue> m_groupes;
    QHash<QString, Salle> m_salles;
    QSet<QString> m_masquees;
    QString m_selection;
    bool m_themeStade = false;
};
