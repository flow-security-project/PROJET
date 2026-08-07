#pragma once

#include <QMainWindow>

#include "data/DataSource.h"
#include "data/DemoSource.h"
#include "data/MqttSource.h"
#include "ui/AlertPanel.h"
#include "ui/SalleDetail.h"
#include "ui/SalleGrid.h"
#include "ui/StatusBar.h"

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSourceChange();
    void connecterMqtt();
    void onSalleMiseAJour(const QString& id);
    void onAlerteAjoutee(const Alerte& a);
    void onAlerteModifiee(const Alerte& a);
    void onConfigConfirmee(const QString& salleId, const QString& detail, int latenceMs);
    void onTestRetour(const QString& salleId, const QString& composant,
                      bool ok, int latenceMs);
    void onEvacuationGlobale(bool active);
    void acquitter(const QString& salleId, quint64 ts);

private:
    void basculerSource(DataSource* src);

    QComboBox* m_sourceBox = nullptr;
    QLineEdit* m_brokerIp = nullptr;
    QLineEdit* m_brokerPort = nullptr;
    QPushButton* m_btnConnecter = nullptr;
    QPlainTextEdit* m_log = nullptr;

    StatusBar* m_status = nullptr;
    SalleGrid* m_grid = nullptr;
    SalleDetail* m_detail = nullptr;
    AlertPanel* m_alertPanel = nullptr;

    DemoSource* m_demo = nullptr;
    MqttSource* m_mqtt = nullptr;
    DataSource* m_source = nullptr;

    QHash<QString, Salle> m_salles;
    bool m_evacGlobale = false;
};
