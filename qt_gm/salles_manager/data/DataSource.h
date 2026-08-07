#pragma once

#include <QHash>
#include <QObject>

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
    virtual void getHauteurPorte(const QString& salleId) = 0;
    virtual void actualiserSalle(const QString& salleId) = 0;

    const QHash<QString, Salle>& salles() const { return m_salles; }

signals:
    void salleAjoutee(const QString& id);
    void salleMiseAJour(const QString& id);
    void hauteurPorteMesuree(const QString& id, double centimetres,
                             bool succes, const QString& note);
    void statutSource(bool connecte, const QString& note);
    void logAppend(const QString& ligne);
    void erreur(const QString& message);

protected:
    QHash<QString, Salle> m_salles;
};
