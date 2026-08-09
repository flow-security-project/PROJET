#include <QApplication>
#include <QDialog>
#include <QFile>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPalette>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include "data/DemoSource.h"
#include "models/AlerteModel.h"
#include "models/Salle.h"
#include "ui/MainWindow.h"
#include "ui/SalleGrid.h"
#include "ui/SallesWidget.h"

static QString chercherIdScenario(int scenario, int nbEssais = 400);
static void testAnticipation(MainWindow& window);

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
                        qApp->setStyleSheet(full);
                        QCoreApplication::processEvents();
                    }
                    window.grab().save(QStringLiteral("/tmp/opencode/app.png"));
                    QTimer::singleShot(300, [&window]() {
                        testAnticipation(window);
                    });
                });
            }
        });
    }

    return app.exec();
}

static QString chercherIdScenario(int scenario, int nbEssais)
{
    for (int i = 1; i <= nbEssais; ++i) {
        const QString candidat = QStringLiteral("A%1").arg(i, 3, 10, QLatin1Char('0'));
        if (int(qHash(candidat)) % 5 == scenario)
            return candidat;
    }
    return QString();
}

static void testAnticipation(MainWindow& window)
{
    qInfo() << "[TEST] debut";
    auto* salles = window.findChild<SallesWidget*>();
    if (!salles) {
        qInfo() << "[TEST] salles introuvable";
        return;
    }

    auto* demo = new DemoSource(salles);
    const QString salleId = chercherIdScenario(1);  // montée rapide (anticipation)
    const QString salleBrusque = chercherIdScenario(4); // sortie brusque (F3)
    Salle s;
    s.id = salleId;
    s.nom = QStringLiteral("Amphi Anticipation");
    s.capacite = 30;
    s.horaireDebut = QStringLiteral("07:00");
    s.horaireFin = QStringLiteral("22:00");
    demo->creerSalle(s);
    Salle b;
    b.id = salleBrusque;
    b.nom = QStringLiteral("Amphi Sortie Brusque");
    b.capacite = 30;
    b.horaireDebut = QStringLiteral("07:00");
    b.horaireFin = QStringLiteral("22:00");
    demo->creerSalle(b);
    salles->setSource(demo);
    qInfo() << "[TEST] salles creees:" << salleId << salleBrusque;

    QTimer::singleShot(800, [salleId, salleBrusque, salles]() {
        qInfo() << "[TEST] etape 1";
        auto* grid = salles->findChild<SalleGrid*>();
        if (!grid) {
            qInfo() << "[TEST] grid introuvable";
            return;
        }
        QWidget* card = nullptr;
        for (QWidget* w : grid->findChildren<QWidget*>()) {
            if (w->objectName() == QStringLiteral("salleCard")
                && w->property("salleId").toString() == salleId) {
                card = w;
                break;
            }
        }
        qInfo() << "[TEST] carte trouvee:" << (card != nullptr);
        if (card) {
            QMouseEvent ev(QEvent::MouseButtonPress,
                           QPointF(12, 12), QPointF(12, 12),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(card, &ev);
        }
        for (QPushButton* bouton : salles->findChildren<QPushButton*>()) {
            if (bouton->text() == QStringLiteral("Afficher la courbe")) {
                bouton->click();
                qInfo() << "[TEST] courbe ouverte";
                break;
            }
        }
        QTimer::singleShot(26000, [salleId, salleBrusque, salles]() {
            qInfo() << "[TEST] etape 2";
            if (auto* demo = salles->findChild<DemoSource*>()) {
                const Salle salle = demo->salles().value(salleId);
                qInfo().noquote()
                    << QStringLiteral("[TEST] occupation=%1 cap=%2 pente=%3 pers/min "
                                      "anticipation=%4 min densHist=%5")
                           .arg(salle.occupation)
                           .arg(salle.capacite)
                           .arg(salle.penteTendance, 0, 'f', 2)
                           .arg(salle.anticipationMin)
                           .arg(salle.densHist.size());
            }
            if (auto* dialogue = salles->findChild<QDialog*>()) {
                qInfo() << "[TEST] dialogue trouve, capture";
                dialogue->grab().save(QStringLiteral("/tmp/opencode/detail_anticipation.png"));
            }
            salles->grab().save(QStringLiteral("/tmp/opencode/config_reseau.png"));
            QTimer::singleShot(54000, [salleBrusque, salles]() {
                qInfo() << "[TEST] etape 3 (alerte F3)";
                if (auto* demo = salles->findChild<DemoSource*>()) {
                    const Salle salle = demo->salles().value(salleBrusque);
                    qInfo() << "[TEST] fluxSortieAnormal=" << salle.fluxSortieAnormal
                            << "pts=" << salle.fluxSortieHist.size();
                }
                if (auto* modele = salles->findChild<AlerteModel*>()) {
                    int nbFlux = 0;
                    for (const Alerte& a : modele->alertes()) {
                        if (a.type == QStringLiteral("flux_sortie"))
                            ++nbFlux;
                    }
                    qInfo() << "[TEST] alertes flux_sortie dans le modèle:" << nbFlux;
                }
                if (auto* dialogue = salles->findChild<QDialog*>()) {
                    dialogue->grab().save(QStringLiteral("/tmp/opencode/detail_alerte.png"));
                }
                salles->grab().save(QStringLiteral("/tmp/opencode/alertes_panel.png"));
                qInfo() << "[TEST] etape 4 (suppression salle)";
                for (QWidget* w : salles->findChildren<QWidget*>()) {
                    if (w->objectName() == QStringLiteral("salleCard")
                        && w->property("salleId").toString() == salleBrusque) {
                        QMouseEvent ev(QEvent::MouseButtonPress,
                                       QPointF(12, 12), QPointF(12, 12),
                                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                        QCoreApplication::sendEvent(w, &ev);
                        break;
                    }
                }
                for (QPushButton* bouton : salles->findChildren<QPushButton*>()) {
                    if (bouton->text() == QStringLiteral("Supprimer la salle")) {
                        bouton->click();
                        break;
                    }
                }
                QTimer::singleShot(400, [salleBrusque, salles]() {
                    if (auto* box = salles->findChild<QMessageBox*>()) {
                        qInfo() << "[TEST] confirmation presente";
                        box->grab().save(QStringLiteral("/tmp/opencode/suppr_confirmation.png"));
                        if (auto* bouton = box->button(QMessageBox::Yes))
                            bouton->click();
                    } else {
                        qInfo() << "[TEST] confirmation ABSENTE";
                    }
                    QTimer::singleShot(600, [salleBrusque, salles]() {
                        bool absent = true;
                        if (auto* demo = salles->findChild<DemoSource*>())
                            absent = !demo->salles().contains(salleBrusque);
                        bool carteAbsente = true;
                        for (QWidget* w : salles->findChildren<QWidget*>()) {
                            if (w->objectName() == QStringLiteral("salleCard")
                                && w->property("salleId").toString() == salleBrusque) {
                                carteAbsente = false;
                                break;
                            }
                        }
                        qInfo() << "[TEST] salle supprimee du modele:" << absent
                                << "carte retiree:" << carteAbsente;
                        salles->grab().save(QStringLiteral("/tmp/opencode/suppr_apres.png"));
                        QCoreApplication::quit();
                    });
                });
            });
        });
    });
}
