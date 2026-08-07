#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QPushButton>
#include <QTimer>

#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption roomIdOption("room-id", "Identifiant de la salle", "id");
    const QCommandLineOption roomNameOption("room-name", "Nom de la salle", "nom");
    const QCommandLineOption roomCapacityOption("room-capacity", "Capacité de la salle", "capacite");
    const QCommandLineOption roomDataOption("room-data", "Fichier JSON de synchronisation", "fichier");
    parser.addOption(roomIdOption);
    parser.addOption(roomNameOption);
    parser.addOption(roomCapacityOption);
    parser.addOption(roomDataOption);
    parser.process(app);

    QFile f(":/styles/charte.qss");
    if (f.open(QFile::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(f.readAll()));

    MainWindow w;
    const bool ouvertureDirecteSalle = parser.isSet(roomIdOption);
    if (!ouvertureDirecteSalle)
        w.show();

    if (parser.isSet(roomIdOption)) {
        SalleGm salle;
        salle.id = parser.value(roomIdOption);
        salle.nom = parser.value(roomNameOption);
        if (salle.nom.isEmpty())
            salle.nom = salle.id;
        salle.capacite = parser.value(roomCapacityOption).toInt();
        if (salle.capacite <= 0)
            salle.capacite = 30;
        const QString fichierSynchronisation = parser.value(roomDataOption);
        QTimer::singleShot(0, &w, [&w, salle, fichierSynchronisation]() {
            w.ouvrirSalleDepuisArguments(salle, fichierSynchronisation);
        });
    }

    if (qEnvironmentVariableIsSet("I_PLUS_PLUS_SHOT")) {
        QTimer::singleShot(9000, &w, [&w]() {
            w.grab().save("/tmp/opencode/qt_gm_main.png");
            auto* btn = w.findChild<QPushButton*>("btnSalle");
            if (btn)
                btn->click();
            QTimer::singleShot(3000, &w, []() {
                for (QWidget* tl : QApplication::topLevelWidgets())
                    if (tl->windowTitle().contains("Salle"))
                        tl->grab().save("/tmp/opencode/qt_gm_salle.png");
                QApplication::quit();
            });
        });
    }
    return app.exec();
}
