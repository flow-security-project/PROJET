#include <QApplication>
#include <QFile>
#include <QPushButton>
#include <QTimer>

#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QFile f(":/styles/charte.qss");
    if (f.open(QFile::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(f.readAll()));

    MainWindow w;
    w.show();
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
