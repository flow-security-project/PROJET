#pragma once

#include <QHash>
#include <QObject>
#include <QString>

#include "models/SalleGm.h"

class GmSource : public QObject
{
    Q_OBJECT

public:
    explicit GmSource(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    virtual ~GmSource() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual QString type() const = 0;

    const QHash<QString, SalleGm>& salles() const { return m_salles; }

signals:
    void salleAjoutee(const QString& id);
    void salleMiseAJour(const QString& id);
    void statutMqtt(bool connecte, const QString& note);
    void logAppend(const QString& ligne);

protected:
    void creerSalle(const SalleGm& s)
    {
        const bool nouveau = !m_salles.contains(s.id);
        m_salles.insert(s.id, s);
        if (nouveau) {
            m_salles[s.id].pushHistorique();
            emit salleAjoutee(s.id);
        } else {
            m_salles[s.id].pushHistorique();
            emit salleMiseAJour(s.id);
        }
    }

    void mettreAjourEtat(const QString& id, int occupation, double densite, bool enLigne)
    {
        if (!m_salles.contains(id))
            return;
        SalleGm& s = m_salles[id];
        s.occupation = occupation;
        s.densite = densite;
        s.enLigne = enLigne;
        s.pushHistorique();
        emit salleMiseAJour(id);
    }

    void compterPassage(const QString& id, const QString& direction)
    {
        if (!m_salles.contains(id))
            return;
        SalleGm& s = m_salles[id];
        if (direction == "entree")
            s.nbEntrees++;
        else if (direction == "sortie")
            s.nbSorties++;
        emit salleMiseAJour(id);
    }

    void logger(const QString& ligne) { emit logAppend(ligne); }

protected:
    QHash<QString, SalleGm> m_salles;
};
