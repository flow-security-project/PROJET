#include "data/DataSource.h"

void DataSource::creerGroupe(const Groupe& groupe)
{
    m_groupes.insert(groupe.id, groupe);
    emit groupeAjoute(groupe.id);
    emit logAppend(QStringLiteral("GROUPE %1 : stade « %2 » créé (mode %3)")
                       .arg(groupe.id, groupe.nom,
                            groupe.mode == ModeFlux::Uni ? QStringLiteral("UNI-MARKET")
                                                         : QStringLiteral("MULTI-MARKET")));
}

void DataSource::modifierGroupe(const Groupe& groupe)
{
    if (!m_groupes.contains(groupe.id))
        return;
    m_groupes.insert(groupe.id, groupe);
    emit groupeMiseAJour(groupe.id);
    emit logAppend(QStringLiteral("GROUPE %1 : stade « %2 » modifié")
                       .arg(groupe.id, groupe.nom));
}

void DataSource::supprimerGroupe(const QString& id)
{
    if (!m_groupes.contains(id))
        return;

    int nbMembres = 0;
    for (auto it = m_salles.cbegin(); it != m_salles.cend(); ++it) {
        if (it.value().groupeId == id) {
            ++nbMembres;
            supprimerSalle(it.key());
        }
    }

    const Groupe groupe = m_groupes.take(id);
    emit groupeSupprime(id);
    emit logAppend(QStringLiteral("GROUPE %1 : stade « %2 » supprimé avec %3 porte(s)")
                       .arg(groupe.id, groupe.nom)
                       .arg(nbMembres));
}
