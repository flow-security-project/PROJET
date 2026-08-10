#pragma once

#include <algorithm>
#include <cmath>

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
    int nbPersonnesEstime = -1;
    double tendance = 0.0;
    double penteTendance = 0.0;
    int anticipationMin = -1;
    qint64 dernierHeartbeatMs = 0;
    int uptimeS = 0;
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

    QVector<double> fluxSortieHist;
    bool fluxSortieAnormal = false;
    qint64 derniereAlerteFluxSortieMs = 0;

    struct DetectionFluxSortie
    {
        bool alerte = false;
        double debit = 0.0;
        double mu = 0.0;
        double sigma = 0.0;
        double seuil = 0.0;
        int points = 0;
    };

    DetectionFluxSortie majDetectionFluxSortie(double debitPersMin, qint64 maintenantMs,
                                               int fenetreSec = 600, int minPoints = 60,
                                               double facteurSigma = 3.0,
                                               double minDebit = 5.0)
    {
        DetectionFluxSortie resultat;
        resultat.debit = debitPersMin;
        fluxSortieHist.append(debitPersMin);
        while (fluxSortieHist.size() > fenetreSec)
            fluxSortieHist.removeFirst();

        const int n = fluxSortieHist.size();
        resultat.points = n;
        if (n < minPoints) {
            fluxSortieAnormal = false;
            return resultat;
        }

        double somme = 0.0;
        for (double valeur : fluxSortieHist)
            somme += valeur;
        resultat.mu = somme / double(n);
        double variance = 0.0;
        for (double valeur : fluxSortieHist)
            variance += (valeur - resultat.mu) * (valeur - resultat.mu);
        resultat.sigma = std::sqrt(variance / double(n));
        resultat.seuil = resultat.mu + facteurSigma * resultat.sigma;

        const bool declare = debitPersMin >= minDebit
                             && debitPersMin > resultat.seuil;
        resultat.alerte = declare && !fluxSortieAnormal;
        fluxSortieAnormal = declare;
        if (resultat.alerte)
            derniereAlerteFluxSortieMs = maintenantMs;
        return resultat;
    }

    double taux() const
    {
        return capacite > 0 && occupation >= 0
                   ? double(occupation) / double(capacite)
                   : 0.0;
    }

    void mettreAJourAnticipation(int fenetrePoints = 120)
    {
        const int count = occHist.size();
        if (count < 5) {
            penteTendance = 0.0;
            anticipationMin = -1;
            return;
        }

        const int fenetre = qMin(fenetrePoints, count);
        const int debut = count - fenetre;
        double sommeX = 0.0, sommeY = 0.0, sommeXY = 0.0, sommeXX = 0.0;
        for (int i = debut; i < count; ++i) {
            const double x = double(i - debut);
            const double y = occHist.at(i);
            sommeX += x;
            sommeY += y;
            sommeXY += x * y;
            sommeXX += x * x;
        }
        const double n = double(fenetre);
        const double denom = n * sommeXX - sommeX * sommeX;
        double pente = 0.0;
        if (denom > 1e-9)
            pente = (n * sommeXY - sommeX * sommeY) / denom;
        penteTendance = pente * 60.0;

        if (occupation >= capacite) {
            anticipationMin = 0;
        } else if (penteTendance <= 0.05) {
            anticipationMin = -1;
        } else {
            const double restant = double(capacite - qMax(0, occupation));
            anticipationMin = int(qCeil(restant / penteTendance));
        }
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
