#pragma once

#include <QDialog>
#include <QSet>

#include "models/Groupe.h"

class DataSource;
class QLabel;
class SalleConfigWidget;
class SalleGrid;
class QVBoxLayout;

class StadeWidget : public QDialog
{
    Q_OBJECT

public:
    explicit StadeWidget(DataSource* source, const QString& groupeId,
                         QWidget* parent = nullptr);

signals:
    void detailPorteDemande(const QString& salleId);

private slots:
    void actualiser();

private:
    void actualiserHeader(const Groupe& groupe);
    void actualiserPortes();
    void actualiserListeMasquees();
    void afficherNouvellePorte();

    DataSource* m_source = nullptr;
    QString m_groupeId;
    QLabel* m_titre = nullptr;
    QLabel* m_sousTitre = nullptr;
    QLabel* m_occupation = nullptr;
    QLabel* m_statut = nullptr;
    QLabel* m_redirection = nullptr;
    QLabel* m_vide = nullptr;
    SalleGrid* m_grid = nullptr;
    SalleConfigWidget* m_config = nullptr;
    QSet<QString> m_vuesPortes;
};
