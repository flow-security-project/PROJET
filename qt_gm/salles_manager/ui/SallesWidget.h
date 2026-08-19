#pragma once

#include <QHash>
#include <QPointer>
#include <QWidget>

#include "models/Alerte.h"
#include "models/Groupe.h"
#include "models/Salle.h"

class AlerteModel;
class AlertePanelWidget;
class DataSource;
class HistoryManager;
class AppelManager;
class QDialog;
class QLabel;
class QPushButton;
class SalleConfigWidget;
class SalleGrid;
class SpeakManager;
class StadeWidget;

class SallesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SallesWidget(QWidget* parent = nullptr);

    void setSource(DataSource* source);
    SpeakManager* speakManager() const { return m_speak; }
    AppelManager* appelManager() const { return m_appel; }

signals:
    void statutChanged(const QString& texte);

private slots:
    void onSalleAjoutee(const QString& id);
    void onSalleSupprimee(const QString& id);
    void onSalleMiseAJour(const QString& id);
    void onGroupeAjoute(const QString& id);
    void onGroupeSupprime(const QString& id);
    void onGroupeMiseAJour(const QString& id);
    void onSalleSelectionnee(const QString& id);
    void onGroupeSelectionnee(const QString& id);
    void onCreerDemandee(const Salle& salle);
    void onGroupeCreerDemande(const Groupe& groupe);
    void onModificationDemandee(const Salle& salle);
    void onMesureDemandee(const QString& id);
    void onActualisationDemandee(const QString& id);
    void onMasquageDemande(const QString& id);
    void onSuppressionDemande(const QString& id);
    void onRestaurationDemandee(const QString& id);
    void onCourbeDemandee(const Salle& salle);
    void onAlerte(const Alerte& alerte);
    void onVoirDetailAlerte(const QString& salleId, quint64 ts);
    void onSourceErreur(const QString& message);
    void onSourceLog(const QString& message);
    void onHauteurMesuree(const QString& id, double centimetres,
                          bool succes, const QString& note);
    void onPassageValide(const QString& salleId, const QString& direction,
                         qint64 timestampMs);
    void onAnnonce(const QString& texte);
    void onDemoRapide();

private:
    void connecterSource(DataSource* source);
    void synchroniserSalles();
    void actualiserListeMasquees();
    void mettreAJourGroupe(const QString& id);
    void ouvrirStade(const QString& id);
    void confirmerSuppressionSalle(const QString& id);

    DataSource* m_source = nullptr;
    HistoryManager* m_history = nullptr;
    SalleGrid* m_grid = nullptr;
    SalleConfigWidget* m_config = nullptr;
    AlerteModel* m_modeleAlertes = nullptr;
    AlertePanelWidget* m_panelAlertes = nullptr;
    SpeakManager* m_speak = nullptr;
    AppelManager* m_appel = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_boutonDemoRapide = nullptr;
    QPointer<QDialog> m_detailDialog;
    QPointer<StadeWidget> m_stadeDialog;
    QHash<QString, QString> m_groupeSalle;  // id salle -> id groupe (avant suppression)
    QString m_detailSalleId;
    QString m_selectionId;
};
