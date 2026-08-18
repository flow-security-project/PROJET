#pragma once

#include <algorithm>
#include <cmath>
#include <QtMath>

#include <QString>
#include <QVector>
#include <QDateTime>

#include "models/Groupe.h"

struct Salle
{
    QString id;
    QString nom;
    QString groupeId;      // vide = salle indépendante
    ModeFlux modeFlux = ModeFlux::Multi;
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
    bool intrusionActive = false;
    double intrusionDureeS = 0.0;
    QString lcdLigne1;
    QString lcdLigne2;
    QString ledCouleur = QStringLiteral("eteint");
    QString ledCouleurConfirmee = QStringLiteral("eteint");
    QString ledMode = QStringLiteral("normal");

    QString decisionFlux;          // "normal" | "attente" | "redirection"
    QString redirectionVers;       // porte de destination en UNI
    double attenteEstimeeMin = -1.0;  // -1 = indéterminée

    QVector<double> occHist;
    QVector<double> densHist;
    QVector<double> entHist;
    QVector<double> sortHist;
    QVector<qint64> timeHist;

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

    void pushHistorique(qint64 timestampMs = 0)
    {
        if (timestampMs <= 0)
            timestampMs = QDateTime::currentMSecsSinceEpoch();
        timeHist.append(timestampMs);
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
        while (timeHist.size() > maxPoints)
            timeHist.removeFirst();
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
        if (intrusionActive)
            return QStringLiteral("INTRUSION ACTIVE");
        return QStringLiteral("EN LIGNE");
    }

    QString couleurLed() const
    {
        if (enAttente || !enLigne)
            return QStringLiteral("eteint");
        if (evacuationActive || intrusionActive)
            return QStringLiteral("rouge");

        const double t = taux();
        if (t >= 0.95)
            return QStringLiteral("rouge");
        if (t >= 0.80)
            return QStringLiteral("orange");
        if (t >= 0.60)
            return QStringLiteral("jaune");
        return QStringLiteral("vert");
    }

    struct ChangementAffichage
    {
        bool ledChanged = false;
        bool lcdChanged = false;
    };

    ChangementAffichage majAffichageLedLcd()
    {
        ChangementAffichage res;

        // LED
        const QString targetLed = couleurLed();
        const QString targetMode = (evacuationActive) ? QStringLiteral("stroboscope") : QStringLiteral("normal");
        if (targetLed != ledCouleur || targetMode != ledMode) {
            ledCouleur = targetLed;
            ledMode = targetMode;
            res.ledChanged = true;
        }

        // LCD
        QString l1, l2;
        if (enAttente) {
            l1 = QStringLiteral("%1  ATTENTE").arg(id);
            l2 = QStringLiteral("ATTENTE CONFIRM.");
        } else if (!enLigne) {
            l1 = QStringLiteral("%1  HORS LIGNE").arg(id);
            l2 = QStringLiteral("CONNEXION...");
        } else if (evacuationActive) {
            l1 = QStringLiteral("EVACUATION ->");
            l2 = QStringLiteral("SORTEZ PAR LA PO");
        } else if (intrusionActive) {
            l1 = QStringLiteral("%1 %2/%3").arg(id).arg(qMax(0, occupation)).arg(capacite);
            l2 = QStringLiteral("INTRUSION DETECT");
        } else {
            l1 = QStringLiteral("%1 %2/%3").arg(id).arg(qMax(0, occupation)).arg(capacite);
            l2 = QStringLiteral("OK %1-%2").arg(horaireDebut).arg(horaireFin);
        }
        l1 = l1.left(16).leftJustified(16, ' ');
        l2 = l2.left(16).leftJustified(16, ' ');

        if (l1 != lcdLigne1 || l2 != lcdLigne2) {
            lcdLigne1 = l1;
            lcdLigne2 = l2;
            res.lcdChanged = true;
        }

        return res;
    }
};
