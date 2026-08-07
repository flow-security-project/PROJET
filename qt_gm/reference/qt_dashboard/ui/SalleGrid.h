#pragma once

#include <QHash>
#include <QLabel>
#include <QWidget>

#include "data/Salle.h"

class SalleGrid : public QWidget
{
    Q_OBJECT

public:
    explicit SalleGrid(QWidget* parent = nullptr);

    void majSalle(const Salle& s);
    void selectionner(const QString& id);

signals:
    void salleSelectionnee(const QString& id);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct Carte {
        QWidget* widget = nullptr;
        QLabel* led = nullptr;
        QLabel* titre = nullptr;
        QLabel* occ = nullptr;
        QLabel* regime = nullptr;
        QLabel* evac = nullptr;
    };

    void construireCarte(const QString& id, const QString& nom);
    void mettreAJourCarte(const QString& id);

    QHash<QString, Carte> m_cartes;
    QHash<QString, Salle> m_salles;
    QString m_sel;
};
