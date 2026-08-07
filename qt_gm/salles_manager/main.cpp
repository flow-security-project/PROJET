#include <QApplication>
#include <QFile>
#include <QPalette>

#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#FFFFFF"));
    palette.setColor(QPalette::Base, QColor("#FFFFFF"));
    palette.setColor(QPalette::AlternateBase, QColor("#F5F5F5"));
    palette.setColor(QPalette::Button, QColor("#FFFFFF"));
    palette.setColor(QPalette::Text, QColor("#1A1A1A"));
    palette.setColor(QPalette::WindowText, QColor("#1A1A1A"));
    palette.setColor(QPalette::ButtonText, QColor("#1A1A1A"));
    palette.setColor(QPalette::Highlight, QColor("#E3F2FD"));
    palette.setColor(QPalette::HighlightedText, QColor("#1A1A1A"));
    palette.setColor(QPalette::PlaceholderText, QColor("#999999"));
    app.setPalette(palette);

    QFile styleFile(QStringLiteral(":/styles/charte.qss"));
    if (styleFile.open(QFile::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));

    MainWindow window;
    window.show();
    return app.exec();
}
