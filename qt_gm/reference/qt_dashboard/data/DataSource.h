#pragma once

#include <QHash>
#include <QMap>
#include <QObject>

#include "Alerte.h"
#include "Salle.h"

class DataSource : public QObject
{
    Q_OBJECT

public:
    explicit DataSource(QObject* parent = nullptr);

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual QString type() const = 0; // "demo" | "mqtt"

    virtual void envoyerConfig(const Salle& s) = 0;
    virtual void commanderTest(const QString& salleId, const QString& composant,
                               const QString& valeur) = 0;
    virtual void forcerEvacuation(const QString& salleId, bool actif) = 0;
    virtual void resetAlertesSalle(const QString& salleId) = 0;

    const QHash<QString, Salle>& salles() const { return m_salles; }
    QList<Alerte> listeAlertes() const { return m_alertes.values(); }

    void acquitterAlerte(quint64 ts);

signals:
    void salleMiseAJour(const QString& salleId);
    void alerteAjoutee(const Alerte& a);
    void alerteModifiee(const Alerte& a);
    void statutMqtt(bool connecte, const QString& note);
    void statutAsterisk(const QString& statut);
    void noeudsMaj(int enLigne, int total);
    void evacuationGlobale(bool active);
    void configConfirmee(const QString& salleId, const QString& detail, int latenceMs);
    void testRetour(const QString& salleId, const QString& composant,
                    bool ok, int latenceMs);
    void logAppend(const QString& ligne);

protected:
    void majNoeuds();
    void majEvacuationGlobale();
    void ajouterAlerte(const Alerte& a);
    void modifierAlerte(quint64 ts);

    QHash<QString, Salle> m_salles;
    QMap<quint64, Alerte> m_alertes;
};
