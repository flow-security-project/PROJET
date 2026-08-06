#include "DemoGmSource.h"

#include <QDateTime>

DemoGmSource::DemoGmSource(QObject* parent)
    : GmSource(parent)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &DemoGmSource::onTick);
}

void DemoGmSource::start()
{
    m_tick = 0;
    m_amphiConfig = false;
    m_b204Config = false;
    m_a102Config = false;
    m_salles.clear();
    m_timer.start();
    emit statutMqtt(true, "Démo simulée");
    logger("Mode DÉMO — le binôme publiera les vraies salles en MQTT");
    logger("3 salles simulées : AMPHI-A, B204, A102");
}

void DemoGmSource::stop()
{
    m_timer.stop();
}

void DemoGmSource::onTick()
{
    m_tick++;

    if (!m_amphiConfig) {
        SalleGm s;
        s.id = "AMPHI-A";
        s.nom = "Amphithéâtre A";
        s.capacite = 300;
        s.horaireDebut = "07:00";
        s.horaireFin = "22:00";
        s.enLigne = true;
        creerSalle(s);
        logger("CONFIG  salle/AMPHI-A  cap 300");
        m_amphiConfig = true;
    }
    if (!m_b204Config) {
        SalleGm s;
        s.id = "B204";
        s.nom = "Salle B204";
        s.capacite = 30;
        s.horaireDebut = "07:00";
        s.horaireFin = "22:00";
        s.enLigne = true;
        creerSalle(s);
        logger("CONFIG  salle/B204  cap 30");
        m_b204Config = true;
    }
    if (!m_a102Config) {
        SalleGm s;
        s.id = "A102";
        s.nom = "Salle A102";
        s.capacite = 40;
        s.horaireDebut = "07:00";
        s.horaireFin = "22:00";
        s.enLigne = false;
        creerSalle(s);
        logger("CONFIG  salle/A102  cap 40");
        m_a102Config = true;
    }

    majAmphi(m_tick);
    majB204(m_tick);
    majA102(m_tick);
}

void DemoGmSource::majAmphi(int tick)
{
    const int occ = qMin(232, int(5 + tick * 2.2));
    mettreAjourEtat("AMPHI-A", occ, double(occ) / 300.0, true);
    if (tick % 2 == 0)
        compterPassage("AMPHI-A", "entree");
}

void DemoGmSource::majB204(int tick)
{
    const int occ = qMin(20, int(3 + tick * 0.7));
    mettreAjourEtat("B204", occ, double(occ) / 30.0, true);
    if (tick % 3 == 0)
        compterPassage("B204", "entree");
}

void DemoGmSource::majA102(int tick)
{
    if (tick == 8) {
        mettreAjourEtat("A102", 0, 0.0, true);
        logger("NŒUD  A102  de nouveau en ligne");
        return;
    }
    if (tick < 8)
        return;
    const int occ = qMin(32, int(8 + (tick - 8) * 1.1));
    mettreAjourEtat("A102", occ, double(occ) / 40.0, true);
    if (tick % 3 == 1)
        compterPassage("A102", "entree");
}
