#pragma once

#include <QPointer>
#include <QWidget>

#include "models/Salle.h"

class DataSource;
class QDialog;
class QLabel;
class SalleConfigWidget;
class SalleGrid;

class SallesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SallesWidget(QWidget* parent = nullptr);

    void setSource(DataSource* source);

signals:
    void statutChanged(const QString& texte);

private slots:
    void onSalleAjoutee(const QString& id);
    void onSalleMiseAJour(const QString& id);
    void onSalleSelectionnee(const QString& id);
    void onCreerDemandee(const Salle& salle);
    void onModificationDemandee(const Salle& salle);
    void onMesureDemandee(const QString& id);
    void onActualisationDemandee(const QString& id);
    void onMasquageDemande(const QString& id);
    void onRestaurationDemandee(const QString& id);
    void onCourbeDemandee(const Salle& salle);
    void onSourceErreur(const QString& message);
    void onSourceLog(const QString& message);
    void onHauteurMesuree(const QString& id, double centimetres,
                          bool succes, const QString& note);

private:
    void connecterSource(DataSource* source);
    void synchroniserSalles();
    void actualiserListeMasquees();

    DataSource* m_source = nullptr;
    SalleGrid* m_grid = nullptr;
    SalleConfigWidget* m_config = nullptr;
    QLabel* m_status = nullptr;
    QPointer<QDialog> m_detailDialog;
    QString m_detailSalleId;
    QString m_selectionId;
};
