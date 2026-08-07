#pragma once

#include <QString>
#include <QVector>

struct SalleGm
{
    QString id;
    QString nom;
    int capacite = 0;
    QString horaireDebut;
    QString horaireFin;

    bool enLigne = false;
    int occupation = 0;
    double densite = 0.0;
    int nbEntrees = 0;
    int nbSorties = 0;

    QVector<double> occHist;
    QVector<double> entHist;
    QVector<double> sortHist;

    double taux() const
    {
        return capacite > 0 ? double(occupation) / capacite : 0.0;
    }

    void pushHistorique()
    {
        occHist.append(double(occupation));
        entHist.append(double(nbEntrees));
        sortHist.append(double(nbSorties));
        const int maxPts = 1800;
        if (occHist.size() > maxPts) {
            occHist.remove(0, occHist.size() - maxPts);
            entHist.remove(0, entHist.size() - maxPts);
            sortHist.remove(0, sortHist.size() - maxPts);
        }
    }

    void viderHistorique()
    {
        occHist.clear();
        entHist.clear();
        sortHist.clear();
    }
};
