#pragma once

#include <QHash>
#include <QSet>
#include <QWidget>

#include "models/Salle.h"

class QGridLayout;
class QLabel;
class QProgressBar;
class QFrame;

class SalleGrid : public QWidget
{
    Q_OBJECT

public:
    explicit SalleGrid(QWidget* parent = nullptr);

    void majSalle(const Salle& salle);
    void viderVue();
    void masquerSalle(const QString& id);
    bool restaurerSalle(const QString& id);
    QStringList sallesMasquees() const;

signals:
    void salleSelectionnee(const QString& id);

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
    void mettreAJourCarte(const QString& id);
    void reflow();
    void setSelection(const QString& id);

    QGridLayout* m_layout = nullptr;
    QHash<QString, Carte> m_cartes;
    QHash<QString, Salle> m_salles;
    QSet<QString> m_masquees;
    QString m_selection;
};
