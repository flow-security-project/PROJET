#pragma once

#include <QEvent>
#include <QKeyEvent>
#include <QMainWindow>

class QComboBox;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QLabel;
class MqttSource;
class DemoSource;
class SallesWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void changeEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSourceChange();
    void connecterMqtt();

private:
    QComboBox* m_sourceBox = nullptr;
    QLineEdit* m_brokerIp = nullptr;
    QLineEdit* m_brokerPort = nullptr;
    QCheckBox* m_voix = nullptr;
    QComboBox* m_langueVoix = nullptr;
    QPushButton* m_testVoix = nullptr;
    QPushButton* m_testAppel = nullptr;
    QLabel* m_asteriskBadge = nullptr;
    SallesWidget* m_salles = nullptr;
    DemoSource* m_demo = nullptr;
    MqttSource* m_mqtt = nullptr;
    Qt::WindowStates m_etatAvantPleinEcran;
    bool m_basculePleinEcran = false;
    bool m_restauration = false;
};
