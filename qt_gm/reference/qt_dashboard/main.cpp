#include <QApplication>
#include <QFile>
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
        QTimer::singleShot(4000, &w, [&w]() {
            w.grab().save("/tmp/opencode/dashboard.png");
            QApplication::quit();
        });
    }
    return app.exec();
}
