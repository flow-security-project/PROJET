#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QTimer>
#include <QWidget>

#include "models/Salle.h"
#include "ui/MainWindow.h"
#include "ui/SalleGrid.h"
#include "ui/SallesWidget.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#EEF2F7"));
    palette.setColor(QPalette::Base, QColor("#FFFFFF"));
    palette.setColor(QPalette::AlternateBase, QColor("#F1F5F9"));
    palette.setColor(QPalette::Button, QColor("#FFFFFF"));
    palette.setColor(QPalette::Text, QColor("#1E293B"));
    palette.setColor(QPalette::WindowText, QColor("#1E293B"));
    palette.setColor(QPalette::ButtonText, QColor("#1E293B"));
    palette.setColor(QPalette::Highlight, QColor("#2563EB"));
    palette.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    palette.setColor(QPalette::PlaceholderText, QColor("#94A3B8"));
    app.setPalette(palette);

    QFile styleFile(QStringLiteral(":/styles/charte.qss"));
    if (styleFile.open(QFile::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));

    MainWindow window;
    window.show();

    if (qEnvironmentVariableIsSet("SM_SHOT")) {
        QTimer::singleShot(2500, [&window]() {
            if (auto* grid = window.findChild<SalleGrid*>()) {
                const auto mk = [](const QString& id, const QString& nom, int occ,
                                   int cap, bool enLigne, bool enAttente = false,
                                   bool evac = false) {
                    Salle s;
                    s.id = id;
                    s.nom = nom;
                    s.occupation = occ;
                    s.capacite = cap;
                    s.enLigne = enLigne;
                    s.enAttente = enAttente;
                    s.evacuationActive = evac;
                    s.hauteurPorteMesuree = true;
                    s.hauteurPorteCm = 210.0;
                    s.densite = 0.5;
                    s.nbEntrees = 128;
                    s.nbSorties = 96;
                    s.horaireDebut = "07:00";
                    s.horaireFin = "22:00";
                    return s;
                };
                grid->majSalle(mk("B201", "Amphi A", 82, 120, true));
                grid->majSalle(mk("B202", "Labo Info", 21, 30, true, true));
                grid->majSalle(mk("B203", "TD Maths", 28, 30, true));
                grid->majSalle(mk("B204", "Salle Réunion", 6, 12, false));
                grid->majSalle(mk("B205", "Salle Évacuée", 130, 130, true, false, true));
                QTimer::singleShot(1200, [grid, &window]() {
                    if (auto* card = grid->findChild<QWidget*>(QStringLiteral("salleCard"))) {
                        QFile styleFile(QStringLiteral(":/styles/charte.qss"));
                        QString full;
                        if (styleFile.open(QFile::ReadOnly))
                            full = QString::fromUtf8(styleFile.readAll());
                        const int marker = full.indexOf(QStringLiteral("CARTES DE SALLE"));
                        const QString before = full.left(marker);
                        const QString after = full.mid(marker);
                        qApp->setStyleSheet(before);
                        QCoreApplication::processEvents();
                        card->grab().save(QStringLiteral("/tmp/opencode/card_before.png"));
                        qApp->setStyleSheet(QStringLiteral("QFrame#salleCard{background-color:#00FF00;}") + after);
                        QCoreApplication::processEvents();
                        card->grab().save(QStringLiteral("/tmp/opencode/card_after.png"));
                        qApp->setStyleSheet(QString());
                    }
                    window.grab().save(QStringLiteral("/tmp/opencode/app.png"));
                    QCoreApplication::quit();
                });
            }
        });
    }

    return app.exec();
}
