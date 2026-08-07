#pragma once

#include <QComboBox>
#include <QHash>
#include <QTableWidget>
#include <QWidget>

#include "data/Alerte.h"

class AlertesTab : public QWidget
{
    Q_OBJECT

public:
    explicit AlertesTab(QWidget* parent = nullptr);

    void ajouterAlerte(const Alerte& a);
    void modifierAlerte(const Alerte& a);
    void vider();
    int nbAlertes() const { return m_table->rowCount(); }

signals:
    void alerteAcquittee(const QString& salleId, quint64 ts);
    void alerteSelectionnee(quint64 ts);

private:
    QTableWidget* m_table = nullptr;
    QComboBox* m_filtre = nullptr;
    QHash<quint64, int> m_rangeeParTs;
};
