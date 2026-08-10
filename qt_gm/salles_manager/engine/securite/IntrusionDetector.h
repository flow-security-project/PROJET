#pragma once

#include <QString>
#include <QTime>

struct IntrusionResultat
{
    bool horsHoraire = false;
    bool intrusionActive = false;
    bool nouvelleAlerte = false;
    double dureeS = 0.0;
};

class IntrusionDetector
{
public:
    IntrusionDetector();

    void setHoraires(const QString& debut, const QString& fin);
    QString horaireDebut() const { return m_debut; }
    QString horaireFin() const { return m_fin; }

    IntrusionResultat verifier(bool presence, qint64 maintenantMs);
    void reset();

private:
    bool horsHoraire(qint64 maintenantMs) const;

    QString m_debut = QStringLiteral("07:00");
    QString m_fin = QStringLiteral("22:00");
    qint64 m_debutPresenceMs = 0;
    double m_dureePresenceS = 0.0;
    bool m_alerteEmise = false;
    bool m_intrusionActive = false;
};
