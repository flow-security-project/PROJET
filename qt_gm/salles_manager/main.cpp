#include <QApplication>
#include <QDialog>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPalette>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include "data/DemoSource.h"
#include "models/AlerteModel.h"
#include "models/Groupe.h"
#include "models/Salle.h"
#include "ui/MainWindow.h"
#include "ui/SalleGrid.h"
#include "ui/SallesWidget.h"

static QString chercherIdScenario(int scenario, int depart = 1, int nbEssais = 400);
static void testAnticipation(MainWindow& window);
static void testStade(SallesWidget* salles);

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

static QString chercherIdScenario(int scenario, int depart, int nbEssais)
{
    for (int i = depart; i < depart + nbEssais; ++i) {
        const QString candidat = QStringLiteral("A%1").arg(i, 3, 10, QLatin1Char('0'));
        if (int(qHash(candidat)) % 6 == scenario)
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
    const QString salleIntrusion = chercherIdScenario(5); // présence permanente (F11)
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
    Salle intrusion;
    intrusion.id = salleIntrusion;
    intrusion.nom = QStringLiteral("Amphi Intrusion");
    intrusion.capacite = 30;
    // Plage autorisée réduite à 1 min à minuit : hors horaires presque toute la journée
    intrusion.horaireDebut = QStringLiteral("00:00");
    intrusion.horaireFin = QStringLiteral("00:01");
    demo->creerSalle(intrusion);
    salles->setSource(demo);
    qInfo() << "[TEST] salles creees:" << salleId << salleBrusque << salleIntrusion;

    QTimer::singleShot(800, [salleId, salleBrusque, salleIntrusion, salles]() {
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
        QTimer::singleShot(26000, [salleId, salleBrusque, salleIntrusion, salles]() {
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
            QTimer::singleShot(54000, [salleBrusque, salleIntrusion, salles]() {
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
                        QTimer::singleShot(0, bouton, [bouton]() { bouton->click(); });
                        break;
                    }
                }
                QTimer::singleShot(400, [salleBrusque, salleIntrusion, salles]() {
                    if (auto* box = salles->findChild<QMessageBox*>()) {
                        qInfo() << "[TEST] confirmation presente";
                        box->grab().save(QStringLiteral("/tmp/opencode/suppr_confirmation.png"));
                        if (auto* bouton = box->button(QMessageBox::Yes))
                            bouton->click();
                    } else {
                        qInfo() << "[TEST] confirmation ABSENTE";
                    }
                    QTimer::singleShot(600, [salleBrusque, salleIntrusion, salles]() {
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
                        QTimer::singleShot(45000, [salleIntrusion, salles]() {
                            qInfo() << "[TEST] etape 5 (intrusion F11)";
                            if (auto* demo = salles->findChild<DemoSource*>()) {
                                const Salle salle = demo->salles().value(salleIntrusion);
                                qInfo() << "[TEST] intrusionActive=" << salle.intrusionActive
                                        << "duree_s=" << salle.intrusionDureeS
                                        << "occupation=" << salle.occupation;
                            }
                            if (auto* modele = salles->findChild<AlerteModel*>()) {
                                int nbIntrusion = 0;
                                for (const Alerte& a : modele->alertes()) {
                                    if (a.type == QStringLiteral("intrusion"))
                                        ++nbIntrusion;
                                }
                                qInfo() << "[TEST] alertes intrusion dans le modèle:" << nbIntrusion;
                            }
                            salles->grab().save(QStringLiteral("/tmp/opencode/intrusion.png"));
                            testStade(salles);
                        });
                    });
                });
            });
        });
    });
}

static void testStade(SallesWidget* salles)
{
    qInfo() << "[TEST] etape 6 (stade UNI-MARKET)";
    auto* demo = salles->findChild<DemoSource*>();
    if (!demo) {
        qInfo() << "[TEST] demo introuvable";
        QCoreApplication::quit();
        return;
    }

    Groupe stade;
    stade.id = QStringLiteral("STADE1");
    stade.nom = QStringLiteral("Stade Sud");
    stade.mode = ModeFlux::Uni;
    stade.seuilEcart = 0.15;
    demo->creerGroupe(stade);

    const QString porte1 = chercherIdScenario(1, 401);   // montée rapide
    const QString porte2 = chercherIdScenario(1, 801);   // montée rapide
    const QString porte3 = chercherIdScenario(3, 1201);  // descente (reste vide)

    const auto mk = [&stade](const QString& id, const QString& nom, int cap) {
        Salle s;
        s.id = id;
        s.nom = nom;
        s.groupeId = stade.id;
        s.modeFlux = ModeFlux::Uni;
        s.capacite = cap;
        s.horaireDebut = QStringLiteral("07:00");
        s.horaireFin = QStringLiteral("22:00");
        s.hauteurPorteMesuree = true;
        s.hauteurPorteCm = 210.0;
        return s;
    };
    demo->creerSalle(mk(porte1, QStringLiteral("Porte Nord"), 4));
    demo->creerSalle(mk(porte2, QStringLiteral("Porte Est"), 4));
    demo->creerSalle(mk(porte3, QStringLiteral("Porte Ouest"), 10));
    qInfo() << "[TEST] stade cree: portes" << porte1 << porte2 << porte3;

    QTimer::singleShot(55000, [porte1, porte2, porte3, salles]() {
        if (auto* demo = salles->findChild<DemoSource*>()) {
            const Salle p1 = demo->salles().value(porte1);
            const Salle p2 = demo->salles().value(porte2);
            const Salle p3 = demo->salles().value(porte3);
            qInfo().noquote() << QStringLiteral(
                "[TEST] decision p1=%1->%2 attente=%3 | p2=%4->%5 attente=%6 | "
                "p3 occ=%7 decision=%8")
                                     .arg(p1.decisionFlux, p1.redirectionVers)
                                     .arg(p1.attenteEstimeeMin)
                                     .arg(p2.decisionFlux, p2.redirectionVers)
                                     .arg(p2.attenteEstimeeMin)
                                     .arg(p3.occupation)
                                     .arg(p3.decisionFlux);
        }
        bool carteStade = false;
        int cartesMulti = 0;
        for (QWidget* w : salles->findChildren<QWidget*>()) {
            if (w->objectName() == QStringLiteral("salleCard")) {
                ++cartesMulti;
                const QString idCarte = w->property("salleId").toString();
                if (idCarte == porte1 || idCarte == porte2 || idCarte == porte3)
                    qInfo() << "[TEST] ECHEC: porte du stade presente dans la grille MULTI" << idCarte;
            }
            if (w->objectName() == QStringLiteral("stadeCard")
                && w->property("groupeId").toString() == QStringLiteral("STADE1")) {
                carteStade = true;
                QMouseEvent ev(QEvent::MouseButtonPress,
                               QPointF(12, 12), QPointF(12, 12),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QCoreApplication::sendEvent(w, &ev);
            }
        }
        qInfo() << "[TEST] carte stade trouvee:" << carteStade
                << "cartes MULTI restantes:" << cartesMulti;
        QTimer::singleShot(800, [porte1, salles]() {
            auto* dialogue = salles->findChild<QDialog*>(QStringLiteral("STADE1"));
            if (!dialogue) {
                qInfo() << "[TEST] dialogue stade ABSENT";
                QCoreApplication::quit();
                return;
            }
            dialogue->grab().save(QStringLiteral("/tmp/opencode/stade_interface.png"));

            int cartesStade = 0;
            QWidget* cartePorte1 = nullptr;
            for (QWidget* w : dialogue->findChildren<QWidget*>()) {
                if (w->objectName() == QStringLiteral("stadeSalleCard")) {
                    ++cartesStade;
                    if (w->property("salleId").toString() == porte1)
                        cartePorte1 = w;
                }
            }
            qInfo() << "[TEST] cartes portes dans l'interface stade:" << cartesStade;

            if (cartePorte1) {
                QMouseEvent ev(QEvent::MouseButtonPress,
                               QPointF(12, 12), QPointF(12, 12),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QCoreApplication::sendEvent(cartePorte1, &ev);
            }
            QTimer::singleShot(400, [dialogue, porte1, salles]() {
                QString idEditeur;
                for (QLineEdit* le : dialogue->findChildren<QLineEdit*>()) {
                    if (le->placeholderText() == QStringLiteral("Ex. B204")) {
                        idEditeur = le->text();
                        break;
                    }
                }
                qInfo() << "[TEST] editeur stade affiche la porte:" << idEditeur
                        << "attendu:" << porte1;
                dialogue->grab().save(QStringLiteral("/tmp/opencode/stade_porte_edition.png"));
                salles->grab().save(QStringLiteral("/tmp/opencode/stade_grille.png"));
                QCoreApplication::quit();
            });
        });
    });
}
