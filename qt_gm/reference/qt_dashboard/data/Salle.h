#pragma once

#include <QString>
#include <QVector>

struct Salle
{
    QString id;
    QString nom;
    int capacite = 30;
    int occupation = 0;          // -1 = non calculé (mode MQTT brut)
    double densite = 0.0;        // 0..1
    QString regime = "clustering"; // "clustering" | "surface"
    double confiance = 1.0;
    double tendance = 0.0;       // pers/min
    int anticipationMin = -1;    // minutes avant saturation, -1 = aucune
    bool enLigne = true;
    bool evacuationActive = false;
    quint64 evacuationDebutTs = 0;
    QString ledCouleur = "vert";
    QString lcdLigne1;
    QString lcdLigne2;
    QString horaireDebut = "07:00";
    QString horaireFin = "22:00";
    int seuilEvacuation = 95;
    int condAudioPct = 0;        // conditions évacuation (F9)
    int condThermPct = 0;
    int condSurfacePct = 0;
    int scoreFusion = 0;         // 0..3 critères actifs
    int dureeEvacS = 0;
    int tauxEvacuation = 0;      // % personnes évacuées
    int occAvantEvac = 0;

    double tofMm = -1;           // valeurs brutes (mode MQTT)
    double ultraCm = -1;
    double rms = -1;
    double temp = -99;
    double hr = -1;

    QVector<double> occHist;
    QVector<double> densHist;
    static const int HIST_CAP = 300;

    double taux() const { return capacite > 0 ? double(occupation) / capacite : 0.0; }

    void pushHistorique()
    {
        occHist.push_back(occupation > 0 ? double(occupation) : 0.0);
        densHist.push_back(densite);
        if (occHist.size() > HIST_CAP) {
            occHist.removeFirst();
            densHist.removeFirst();
        }
    }

    void majCouleurLed()
    {
        if (!enLigne)            ledCouleur = "gris";
        else if (evacuationActive) ledCouleur = "rouge";
        else {
            const double t = taux();
            if (t >= 0.95)       ledCouleur = "rouge";
            else if (t >= 0.80)  ledCouleur = "orange";
            else if (t >= 0.60)  ledCouleur = "jaune";
            else                 ledCouleur = "vert";
        }
    }

    QString occTexte() const
    {
        return occupation < 0 ? "--/--" : QString("%1/%2").arg(occupation).arg(capacite);
    }
};
