#pragma once

#include <QString>
#include <QStringList>

struct Alerte
{
    quint64 ts = 0;
    QString salleId;
    QString salleNom;
    QString type;        // evacuation | bousculade | saturation | immobile | intrusion | flux_sortie
    QString detail;
    double score = 0.0;
    QStringList capteurs;
    QString appelCible;  // Sécurité | Infirmerie | Gestion | Admin IT
    QString appelStatut; // en_cours | termine | echoue
    QString appelHeure;
    bool acquittee = false;

    QString severite() const
    {
        if (type == "evacuation") return "critique";
        if (type == "immobile")   return "info";
        if (type == "flux_sortie") return "attention";
        return "attention";
    }

    QString typeLibelle() const
    {
        if (type == "evacuation")    return "ÉVACUATION";
        if (type == "bousculade")    return "BOUSCULADE";
        if (type == "saturation")    return "SATURATION";
        if (type == "immobile")      return "PERSONNE IMMOBILE";
        if (type == "intrusion")     return "INTRUSION";
        if (type == "flux_sortie")   return "FLUX SORTIE ANORMAL";
        return type.toUpper();
    }

    QString appelStatutTexte() const
    {
        if (appelStatut == "termine") return "Terminé";
        if (appelStatut == "echoue")  return "Échoué";
        if (appelStatut.isEmpty())    return "—";
        return "En cours";
    }
};
