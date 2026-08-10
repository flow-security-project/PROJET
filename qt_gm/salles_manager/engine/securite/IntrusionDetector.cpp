#include "engine/securite/IntrusionDetector.h"

IntrusionDetector::IntrusionDetector() = default;

void IntrusionDetector::setHoraires(const QString& debut, const QString& fin)
{
    m_debut = debut;
    m_fin = fin;
}

bool IntrusionDetector::horsHoraire(qint64 maintenantMs) const
{
    const QTime heureActuelle = QTime::fromMSecsSinceStartOfDay(maintenantMs % (24LL * 3600 * 1000));
    const QTime debut = QTime::fromString(m_debut, QStringLiteral("HH:mm"));
    const QTime fin = QTime::fromString(m_fin, QStringLiteral("HH:mm"));
    if (!debut.isValid() || !fin.isValid())
        return false;

    if (debut == fin)
        return false;

    if (debut < fin)
        return heureActuelle < debut || heureActuelle >= fin;

    return heureActuelle >= fin && heureActuelle < debut;
}

IntrusionResultat IntrusionDetector::verifier(bool presence, qint64 maintenantMs)
{
    IntrusionResultat resultat;
    resultat.horsHoraire = horsHoraire(maintenantMs);

    if (presence && resultat.horsHoraire) {
        if (m_debutPresenceMs == 0)
            m_debutPresenceMs = maintenantMs;
        m_dureePresenceS = double(maintenantMs - m_debutPresenceMs) / 1000.0;
        resultat.dureeS = m_dureePresenceS;

        if (m_dureePresenceS >= 120.0 && !m_alerteEmise) {
            m_alerteEmise = true;
            m_intrusionActive = true;
            resultat.nouvelleAlerte = true;
        }
    } else {
        m_debutPresenceMs = 0;
        m_dureePresenceS = 0.0;
        m_alerteEmise = false;
        m_intrusionActive = false;
    }

    resultat.intrusionActive = m_intrusionActive;
    return resultat;
}

void IntrusionDetector::reset()
{
    m_debutPresenceMs = 0;
    m_dureePresenceS = 0.0;
    m_alerteEmise = false;
    m_intrusionActive = false;
}
