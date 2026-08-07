#pragma once

#include <QCheckBox>
#include <QHash>
#include <QListWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "data/Alerte.h"

class AlertPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AlertPanel(QWidget* parent = nullptr);

    void ajouterAlerte(const Alerte& a);
    void modifierAlerte(const Alerte& a);
    void vider();

signals:
    void alerteAcquittee(const QString& salleId, quint64 ts);
    void voirDetail(const QString& salleId, quint64 ts);

private:
    void ajouterRangee(const Alerte& a);

    QWidget* m_liste = nullptr;
    QVBoxLayout* m_layListe = nullptr;
    QCheckBox* m_filtreNonAcq = nullptr;
    QHash<quint64, Alerte> m_alertes;
};
