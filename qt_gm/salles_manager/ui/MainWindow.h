#pragma once

#include <QMainWindow>

class QComboBox;
class QLineEdit;
class MqttSource;
class DemoSource;
class SallesWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSourceChange();
    void connecterMqtt();

private:
    QComboBox* m_sourceBox = nullptr;
    QLineEdit* m_brokerIp = nullptr;
    QLineEdit* m_brokerPort = nullptr;
    SallesWidget* m_salles = nullptr;
    DemoSource* m_demo = nullptr;
    MqttSource* m_mqtt = nullptr;
};
