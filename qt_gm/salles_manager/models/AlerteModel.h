#pragma once

#include <algorithm>

#include <QDateTime>
#include <QHash>
#include <QObject>

#include "models/Alerte.h"

class AlerteModel : public QObject
{
    Q_OBJECT

public:
    explicit AlerteModel(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    void ajouter(const Alerte& a)
    {
        Alerte ajout = a;
        if (ajout.ts == 0)
            ajout.ts = QDateTime::currentMSecsSinceEpoch();
        while (m_alertes.contains(ajout.ts))
            ++ajout.ts;
        m_alertes.insert(ajout.ts, ajout);
        emit alerteAjoutee(ajout);
    }

    void modifier(const Alerte& a)
    {
        if (!m_alertes.contains(a.ts))
            return;
        m_alertes.insert(a.ts, a);
        emit alerteModifiee(a);
    }

    void acquitter(quint64 ts, bool acquittee = true)
    {
        if (!m_alertes.contains(ts))
            return;
        Alerte a = m_alertes.value(ts);
        if (a.acquittee == acquittee)
            return;
        a.acquittee = acquittee;
        m_alertes.insert(ts, a);
        emit alerteModifiee(a);
    }

    void vider()
    {
        if (m_alertes.isEmpty())
            return;
        m_alertes.clear();
        emit alerteVidee();
    }

    bool contient(quint64 ts) const { return m_alertes.contains(ts); }

    Alerte alerte(quint64 ts) const { return m_alertes.value(ts); }

    QList<Alerte> alertes() const { return m_alertes.values(); }

    void charger(const QList<Alerte>& alertes)
    {
        for (const Alerte& a : alertes) {
            if (a.ts != 0)
                m_alertes.insert(a.ts, a);
        }
    }

    QList<Alerte> alertesPourSalle(const QString& salleId) const
    {
        QList<Alerte> liste;
        for (const Alerte& a : m_alertes) {
            if (a.salleId == salleId)
                liste.append(a);
        }
        std::sort(liste.begin(), liste.end(),
                  [](const Alerte& gauche, const Alerte& droite) {
                      return gauche.ts < droite.ts;
                  });
        return liste;
    }

signals:
    void alerteAjoutee(const Alerte& a);
    void alerteModifiee(const Alerte& a);
    void alerteVidee();

private:
    QHash<quint64, Alerte> m_alertes;
};
