#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimeZone>
#include <QElapsedTimer>

#include "history/HistoryManager.h"

namespace {

bool verifier(bool condition, const QString& message)
{
    if (condition)
        return true;
    QTextStream(stderr) << "history_smoke: " << message << '\n';
    return false;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    if (!verifier(directory.isValid(), QStringLiteral("répertoire temporaire invalide")))
        return 1;

    const QDateTime noon(QDate::currentDate(), QTime(12, 0),
                         QTimeZone::systemTimeZone());
    const qint64 firstTimestamp = noon.toMSecsSinceEpoch() + 1000;
    const qint64 secondTimestamp = firstTimestamp + 30000;

    Salle salle;
    salle.id = QStringLiteral("B204");
    salle.nom = QStringLiteral("Salle test");
    salle.capacite = 30;
    salle.occupation = 2;
    salle.densite = 0.2;
    salle.nbEntrees = 1;
    salle.regime = QStringLiteral("clustering");

    HistoryManager history(directory.path());
    history.recordSalle(salle, firstTimestamp);
    salle.occupation = 4;
    salle.densite = 0.4;
    salle.nbEntrees = 2;
    history.recordSalle(salle, secondTimestamp);
    history.recordPassage(salle.id, QStringLiteral("entree"), firstTimestamp, salle);
    history.recordPassage(salle.id, QStringLiteral("sortie"), secondTimestamp, salle);

    Alerte alerte;
    alerte.ts = secondTimestamp;
    alerte.salleId = salle.id;
    alerte.salleNom = salle.nom;
    alerte.type = QStringLiteral("flux_sortie");
    alerte.detail = QStringLiteral("Débit, test");
    alerte.capteurs = {QStringLiteral("VL53L5CX")};
    history.recordAlerte(alerte);
    history.flush();

    const HistoryStats stats = history.analyse(
        salle.id, HistoryPeriod::Day, secondTimestamp + 1000);
    if (!verifier(stats.nombrePoints == 1, QStringLiteral("agrégation minute incorrecte"))
        || !verifier(stats.nombreEntrees == 1 && stats.nombreSorties == 1,
                     QStringLiteral("passages absents"))
        || !verifier(stats.pic.occupation == 3,
                     QStringLiteral("moyenne occupation incorrecte"))) {
        return 1;
    }

    const QString samplesPath = QDir(directory.path()).filePath(
        QStringLiteral("salle_B204_history.csv"));
    const QString passagesPath = QDir(directory.path()).filePath(
        QStringLiteral("salle_B204_passages.csv"));
    const QString alertsPath = QDir(directory.path()).filePath(
        QStringLiteral("alertes_history.csv"));
    if (!verifier(QFile::exists(samplesPath), QStringLiteral("CSV séries absent"))
        || !verifier(QFile::exists(passagesPath), QStringLiteral("CSV passages absent"))
        || !verifier(QFile::exists(alertsPath), QStringLiteral("CSV alertes absent"))) {
        return 1;
    }

    const QString exportPath = QDir(directory.path()).filePath(QStringLiteral("export.csv"));
    QString error;
    if (!verifier(history.exportSalleCsv(salle.id, HistoryPeriod::Day, exportPath, &error),
                  error)
        || !verifier(QFileInfo(exportPath).size() > 0,
                     QStringLiteral("export CSV vide"))) {
        return 1;
    }

    HistoryManager reloaded(directory.path());
    if (!verifier(reloaded.alertes().size() == 1,
                  QStringLiteral("alerte non rechargée après redémarrage"))) {
        return 1;
    }

    Salle invalid = salle;
    invalid.id = QStringLiteral("INVALID");
    invalid.occupation = -1;
    invalid.densite = -1.0;
    invalid.confiance = -1.0;
    const qint64 invalidTimestamp = secondTimestamp + 60000;
    history.recordSalle(invalid, invalidTimestamp);
    history.flush();
    const QVector<HistorySample> invalidSamples = history.samples(
        invalid.id, HistoryPeriod::Day, invalidTimestamp + 1000);
    if (!verifier(invalidSamples.size() == 1
                      && !invalidSamples.first().occupationValid
                      && !invalidSamples.first().densiteValid,
                  QStringLiteral("valeur invalide mal conservée"))) {
        return 1;
    }

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < 5000; ++i) {
        Salle bulk = salle;
        bulk.id = QStringLiteral("BULK");
        bulk.occupation = i % 31;
        history.recordPassage(bulk.id,
                              i % 2 == 0 ? QStringLiteral("entree")
                                         : QStringLiteral("sortie"),
                              firstTimestamp + i * 10,
                              bulk);
    }
    history.flush();
    const qint64 elapsedMs = timer.elapsed();
    if (!verifier(history.passages(QStringLiteral("BULK"), HistoryPeriod::Day,
                                   firstTimestamp + 60000).size() == 5000,
                  QStringLiteral("perte de passages en volume"))
        || !verifier(elapsedMs < 5000,
                     QStringLiteral("écriture de volume trop lente"))) {
        return 1;
    }
    return 0;
}
