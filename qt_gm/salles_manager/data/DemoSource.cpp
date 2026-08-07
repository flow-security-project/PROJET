#include "DemoSource.h"

#include <QRandomGenerator>

DemoSource::DemoSource(QObject* parent)
    : DataSource(parent)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &DemoSource::onTick);
}

void DemoSource::start()
{
    m_tick = 0;
    m_timer.start();
    emit statutSource(true, QStringLiteral("Démo locale"));
    emit logAppend(QStringLiteral("Mode DÉMO démarré — créez une salle pour commencer"));
}

void DemoSource::stop()
{
    m_timer.stop();
}

void DemoSource::creerSalle(const Salle& salle)
{
    if (salle.id.trimmed().isEmpty()) {
        emit erreur(QStringLiteral("L'identifiant de salle est obligatoire."));
        return;
    }
    if (m_salles.contains(salle.id)) {
        emit erreur(QStringLiteral("L'identifiant %1 existe déjà.").arg(salle.id));
        return;
    }

    Salle s = salle;
    s.enLigne = true;
    s.enAttente = false;
    s.occupation = 0;
    s.densite = 0.0;
    s.pushHistorique();
    m_salles.insert(s.id, s);
    emit salleAjoutee(s.id);
    emit logAppend(QStringLiteral("SALLE créée — %1 (%2)").arg(s.id, s.nom));
}

void DemoSource::modifierSalle(const Salle& salle)
{
    if (!m_salles.contains(salle.id)) {
        emit erreur(QStringLiteral("Salle inconnue : %1").arg(salle.id));
        return;
    }

    Salle updated = salle;
    const Salle previous = m_salles.value(salle.id);
    updated.occupation = previous.occupation;
    updated.densite = previous.densite;
    updated.tendance = previous.tendance;
    updated.nbEntrees = previous.nbEntrees;
    updated.nbSorties = previous.nbSorties;
    updated.occHist = previous.occHist;
    updated.densHist = previous.densHist;
    updated.entHist = previous.entHist;
    updated.sortHist = previous.sortHist;
    updated.enLigne = true;
    updated.enAttente = false;
    m_salles.insert(updated.id, updated);
    emit salleMiseAJour(updated.id);
    emit logAppend(QStringLiteral("CONFIGURATION modifiée — %1").arg(updated.id));
}

void DemoSource::getHauteurPorte(const QString& salleId)
{
    if (salleId.trimmed().isEmpty()) {
        emit erreur(QStringLiteral("Saisissez un identifiant avant la mesure."));
        return;
    }

    emit logAppend(QStringLiteral("MESURE hauteur de porte — %1").arg(salleId));
    QTimer::singleShot(650, this, [this, salleId]() {
        const double valeur = 205.0
                              + double(QRandomGenerator::global()->bounded(21));
        if (m_salles.contains(salleId)) {
            Salle& s = m_salles[salleId];
            s.hauteurPorteCm = valeur;
            s.hauteurPorteMesuree = true;
            emit salleMiseAJour(salleId);
        }
        emit hauteurPorteMesuree(salleId, valeur, true,
                                 QStringLiteral("Mesure démo confirmée"));
    });
}

void DemoSource::actualiserSalle(const QString& salleId)
{
    if (!m_salles.contains(salleId))
        return;
    emit salleMiseAJour(salleId);
    emit logAppend(QStringLiteral("ACTUALISATION — %1").arg(salleId));
}

void DemoSource::onTick()
{
    ++m_tick;
    for (auto it = m_salles.begin(); it != m_salles.end(); ++it) {
        Salle& s = it.value();
        const int base = int(qHash(s.id) % uint(qMax(1, s.capacite / 2)));
        const int amplitude = qMax(1, s.capacite / 3);
        s.occupation = qBound(0, base + ((m_tick * 2 + base) % amplitude), s.capacite);
        s.densite = s.taux();
        s.tendance = (m_tick % 12 < 6) ? 1.2 : -0.8;
        if (m_tick % 2 == 0 && s.tendance > 0)
            ++s.nbEntrees;
        if (m_tick % 5 == 0 && s.tendance < 0)
            ++s.nbSorties;
        s.pushHistorique();
        emit salleMiseAJour(s.id);
    }
}
