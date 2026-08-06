#pragma once

#include <QHash>
#include <QMainWindow>

#include "models/SalleGm.h"
#include "ui/SalleWindow.h"

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class GmSource;
class DemoGmSource;
class MqttGmSource;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSalleAjoutee(const QString& id);
    void onSalleMiseAJour(const QString& id);
    void onSourceChange();
    void connecterMqtt();
    void ouvrirSalle(const QString& id);

private:
    void basculerSource(GmSource* src);
    void construireBouton(const QString& id);
    void majBouton(const QString& id);

    QComboBox* m_sourceBox = nullptr;
    QLineEdit* m_brokerIp = nullptr;
    QLineEdit* m_brokerPort = nullptr;
    QPushButton* m_btnConnecter = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QWidget* m_boutonsHote = nullptr;

    QHash<QString, SalleGm> m_salles;
    QHash<QString, QPushButton*> m_boutons;
    QHash<QString, SalleWindow*> m_fenetres;

    DemoGmSource* m_demo = nullptr;
    MqttGmSource* m_mqtt = nullptr;
    GmSource* m_source = nullptr;
};
