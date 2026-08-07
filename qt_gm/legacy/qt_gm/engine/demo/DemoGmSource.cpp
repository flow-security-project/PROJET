#include "DemoGmSource.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <utility>

namespace {
QVector<double> lireHistorique(const QJsonValue& value)
{
    QVector<double> historique;
    const QJsonArray array = value.toArray();
    historique.reserve(array.size());
    for (const QJsonValue& item : array)
        historique.append(item.toDouble());
    return historique;
}
}

DemoGmSource::DemoGmSource(QObject* parent)
    : GmSource(parent)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &DemoGmSource::onTick);
    m_synchronisationTimer.setInterval(250);
    connect(&m_synchronisationTimer, &QTimer::timeout,
            this, &DemoGmSource::onSynchronisationTick);
}

void DemoGmSource::start()
{
    m_tick = 0;
    m_sallesAjoutees.clear();
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
    m_synchronisationTimer.stop();
}

void DemoGmSource::ajouterSalle(const SalleGm& salle)
{
    if (salle.id.isEmpty())
        return;

    SalleGm s = salle;
    s.enLigne = true;
    s.occupation = qMax(0, s.occupation);
    s.pushHistorique();
    m_salles.insert(s.id, s);

    if (s.id != "AMPHI-A" && s.id != "B204" && s.id != "A102")
        m_sallesAjoutees.insert(s.id);
    emit salleAjoutee(s.id);
    logger(QString("SALLE  %1  ajoutée depuis le gestionnaire").arg(s.id));
}

void DemoGmSource::activerSynchronisation(const QString& fichier)
{
    m_synchronisationFichier = fichier;
    m_synchronisationId.clear();
    onSynchronisationTick();
    m_synchronisationTimer.start();
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

    if (m_synchronisationId != "AMPHI-A")
        majAmphi(m_tick);
    if (m_synchronisationId != "B204")
        majB204(m_tick);
    if (m_synchronisationId != "A102")
        majA102(m_tick);

    for (const QString& id : std::as_const(m_sallesAjoutees)) {
        if (id == m_synchronisationId)
            continue;
        if (!m_salles.contains(id))
            continue;
        SalleGm& salle = m_salles[id];
        const int capacite = qMax(1, salle.capacite);
        const int amplitude = qMax(1, capacite / 2);
        const int occupation = qMin(capacite - 1, 3 + ((m_tick * 2) % amplitude));
        mettreAjourEtat(id, occupation, double(occupation) / capacite, true);
        if (m_tick % 2 == 0)
            compterPassage(id, "entree");
    }
}

void DemoGmSource::onSynchronisationTick()
{
    if (m_synchronisationFichier.isEmpty())
        return;

    QFile file(m_synchronisationFichier);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return;

    const QJsonObject object = document.object();
    const QString id = object.value(QStringLiteral("id")).toString();
    if (id.isEmpty())
        return;
    m_synchronisationId = id;

    SalleGm salle = m_salles.value(id);
    salle.id = id;
    salle.nom = object.value(QStringLiteral("nom")).toString(salle.nom);
    salle.capacite = object.value(QStringLiteral("capacite")).toInt(salle.capacite);
    salle.horaireDebut = object.value(QStringLiteral("horaire_debut"))
                             .toString(salle.horaireDebut);
    salle.horaireFin = object.value(QStringLiteral("horaire_fin"))
                           .toString(salle.horaireFin);
    salle.occupation = object.value(QStringLiteral("occupation")).toInt(salle.occupation);
    salle.densite = object.value(QStringLiteral("densite")).toDouble(salle.densite);
    salle.enLigne = object.value(QStringLiteral("en_ligne")).toBool(salle.enLigne);
    salle.nbEntrees = object.value(QStringLiteral("nb_entrees")).toInt(salle.nbEntrees);
    salle.nbSorties = object.value(QStringLiteral("nb_sorties")).toInt(salle.nbSorties);
    salle.occHist = lireHistorique(object.value(QStringLiteral("occ_hist")));
    salle.entHist = lireHistorique(object.value(QStringLiteral("ent_hist")));
    salle.sortHist = lireHistorique(object.value(QStringLiteral("sort_hist")));

    const bool existed = m_salles.contains(id);
    m_salles.insert(id, salle);
    if (existed)
        emit salleMiseAJour(id);
    else
        emit salleAjoutee(id);
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
