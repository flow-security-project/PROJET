#pragma once

#include <QHash>
#include <QObject>

#include "models/Alerte.h"
#include "models/Groupe.h"
#include "models/Salle.h"

class DataSource : public QObject
{
    Q_OBJECT

public:
    explicit DataSource(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
    ~DataSource() override = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual QString type() const = 0;

    virtual void creerSalle(const Salle& salle) = 0;
    virtual void modifierSalle(const Salle& salle) = 0;
    virtual void supprimerSalle(const QString& id) = 0;
    virtual void getHauteurPorte(const QString& salleId) = 0;
    virtual void actualiserSalle(const QString& salleId) = 0;

    const QHash<QString, Salle>& salles() const { return m_salles; }

    void creerGroupe(const Groupe& groupe);
    void modifierGroupe(const Groupe& groupe);
    void supprimerGroupe(const QString& id);
    const QHash<QString, Groupe>& groupes() const { return m_groupes; }

signals:
    void salleAjoutee(const QString& id);
    void salleSupprimee(const QString& id);
    void salleMiseAJour(const QString& id);
    void groupeAjoute(const QString& id);
    void groupeSupprime(const QString& id);
    void groupeMiseAJour(const QString& id);
    void hauteurPorteMesuree(const QString& id, double centimetres,
                             bool succes, const QString& note);
    void statutSource(bool connecte, const QString& note);
    void logAppend(const QString& ligne);
    void erreur(const QString& message);
    void passageValide(const QString& salleId, const QString& direction,
                       qint64 timestampMs);
    void alerte(const Alerte& alerte);

protected:
    QHash<QString, Salle> m_salles;
    QHash<QString, Groupe> m_groupes;
};
