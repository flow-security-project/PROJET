#include "DataSource.h"

DataSource::DataSource(QObject* parent)
    : QObject(parent)
{
}

void DataSource::acquitterAlerte(quint64 ts)
{
    auto it = m_alertes.find(ts);
    if (it == m_alertes.end())
        return;
    it->acquittee = true;
    emit alerteModifiee(it.value());
}

void DataSource::majNoeuds()
{
    int enLigne = 0;
    for (const Salle& s : m_salles)
        if (s.enLigne)
            enLigne++;
    emit noeudsMaj(enLigne, m_salles.size());
}

void DataSource::majEvacuationGlobale()
{
    for (const Salle& s : m_salles)
        if (s.evacuationActive) {
            emit evacuationGlobale(true);
            return;
        }
    emit evacuationGlobale(false);
}

void DataSource::ajouterAlerte(const Alerte& a)
{
    m_alertes.insert(a.ts, a);
    emit alerteAjoutee(a);
    emit logAppend("ALERTE  " + a.salleNom + "  " + a.typeLibelle()
                   + (a.appelCible.isEmpty() ? QString() : "  -> appel " + a.appelCible));
}

void DataSource::modifierAlerte(quint64 ts)
{
    auto it = m_alertes.find(ts);
    if (it != m_alertes.end())
        emit alerteModifiee(it.value());
}
