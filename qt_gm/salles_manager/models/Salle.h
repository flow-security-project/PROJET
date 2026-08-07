#pragma once

#include <QString>
#include <QVector>

struct Salle
{
    QString id;
    QString nom;
    int capacite = 30;
    QString horaireDebut = "07:00";
    QString horaireFin = "22:00";
    int seuilEvacuation = 95;

    double hauteurPorteCm = -1.0;
    bool hauteurPorteMesuree = false;
    bool enLigne = false;
    bool enAttente = false;

    int occupation = -1;
    double densite = 0.0;
    double tendance = 0.0;
    int nbEntrees = 0;
    int nbSorties = 0;
    QString regime = "clustering";
    double confiance = -1.0;
    bool evacuationActive = false;
    QString lcdLigne1;
    QString lcdLigne2;

    QVector<double> occHist;
    QVector<double> densHist;
    QVector<double> entHist;
    QVector<double> sortHist;

    double taux() const
    {
        return capacite > 0 && occupation >= 0
                   ? double(occupation) / double(capacite)
                   : 0.0;
    }

    void pushHistorique()
    {
        occHist.append(occupation >= 0 ? double(occupation) : 0.0);
        densHist.append(densite);
        entHist.append(double(nbEntrees));
        sortHist.append(double(nbSorties));

        constexpr int maxPoints = 1800;
        while (occHist.size() > maxPoints)
            occHist.removeFirst();
        while (densHist.size() > maxPoints)
            densHist.removeFirst();
        while (entHist.size() > maxPoints)
            entHist.removeFirst();
        while (sortHist.size() > maxPoints)
            sortHist.removeFirst();
    }

    QString occupationTexte() const
    {
        return occupation < 0
                   ? QStringLiteral("--/--")
                   : QStringLiteral("%1/%2").arg(occupation).arg(capacite);
    }

    QString statutTexte() const
    {
        if (enAttente)
            return QStringLiteral("EN ATTENTE DE CONFIRMATION");
        if (!enLigne)
            return QStringLiteral("HORS LIGNE");
        if (evacuationActive)
            return QStringLiteral("EVACUATION ACTIVE");
        return QStringLiteral("EN LIGNE");
    }
};
