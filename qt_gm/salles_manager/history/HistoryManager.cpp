#include "history/HistoryManager.h"

#include <algorithm>
#include <utility>

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringConverter>
#include <QTextStream>
#include <QTime>
#include <QTimeZone>
#include <QTimer>

namespace {

constexpr qint64 kMinuteMs = 60LL * 1000LL;

QString csvEscape(QString value)
{
    value.replace(QStringLiteral("\r"), QStringLiteral(" "));
    value.replace(QStringLiteral("\n"), QStringLiteral(" "));
    value.replace(QStringLiteral("\""), QStringLiteral("\"\""));
    return QStringLiteral("\"") + value + QStringLiteral("\"");
}

QStringList csvFields(const QString& line)
{
    QStringList fields;
    QString field;
    bool quoted = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar character = line.at(i);
        if (character == QLatin1Char('"')) {
            if (quoted && i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                field += QLatin1Char('"');
                ++i;
            } else {
                quoted = !quoted;
            }
        } else if (character == QLatin1Char(',') && !quoted) {
            fields.append(field);
            field.clear();
        } else {
            field += character;
        }
    }
    fields.append(field);
    return fields;
}

QString number(double value, int decimals = 6)
{
    return QLocale::c().toString(value, 'f', decimals);
}

double toDouble(const QString& value, double fallback = 0.0)
{
    bool ok = false;
    const double result = QLocale::c().toDouble(value, &ok);
    return ok ? result : fallback;
}

qint64 toInt64(const QString& value, qint64 fallback = 0)
{
    bool ok = false;
    const qint64 result = value.toLongLong(&ok);
    return ok ? result : fallback;
}

int toInt(const QString& value, int fallback = 0)
{
    bool ok = false;
    const int result = value.toInt(&ok);
    return ok ? result : fallback;
}

QString timestampIso(qint64 timestampMs)
{
    return QDateTime::fromMSecsSinceEpoch(timestampMs).toString(Qt::ISODate);
}

QString csvRow(const QStringList& fields)
{
    QStringList escaped;
    escaped.reserve(fields.size());
    for (const QString& field : fields)
        escaped.append(csvEscape(field));
    return escaped.join(QLatin1Char(',')) + QLatin1Char('\n');
}

bool inPeriod(qint64 timestampMs, HistoryPeriod period, qint64 referenceMs)
{
    return timestampMs >= HistoryManager::debutPeriode(period, referenceMs)
           && timestampMs <= (referenceMs == 0
                                  ? QDateTime::currentMSecsSinceEpoch()
                                  : referenceMs);
}

} // namespace

HistoryManager::HistoryManager(const QString& directory, QObject* parent)
    : QObject(parent)
    , m_directory(directory)
    , m_timer(new QTimer(this))
{
    if (m_directory.isEmpty()) {
        m_directory = QDir(QStandardPaths::writableLocation(
                               QStandardPaths::AppDataLocation))
                          .filePath(QStringLiteral("history"));
    }

    ensureDirectory();
    loadAlertes();

    m_timer->setInterval(int(kMinuteMs));
    connect(m_timer, &QTimer::timeout, this, &HistoryManager::onMinute);
    m_timer->start();
}

HistoryManager::~HistoryManager()
{
    flush();
}

void HistoryManager::ensureDirectory()
{
    if (QDir().mkpath(m_directory))
        return;
    emit storageError(QStringLiteral("Impossible de créer le répertoire historique : %1")
                          .arg(m_directory));
}

QString HistoryManager::salleStem(const QString& salleId) const
{
    QString safe = salleId;
    safe.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")),
                 QStringLiteral("_"));
    if (safe.isEmpty())
        safe = QStringLiteral("inconnue");
    return QStringLiteral("salle_%1").arg(safe);
}

QString HistoryManager::samplesPath(const QString& salleId) const
{
    return QDir(m_directory).filePath(salleStem(salleId) + QStringLiteral("_history.csv"));
}

QString HistoryManager::passagesPath(const QString& salleId) const
{
    return QDir(m_directory).filePath(salleStem(salleId) + QStringLiteral("_passages.csv"));
}

QString HistoryManager::alertesPath() const
{
    return QDir(m_directory).filePath(QStringLiteral("alertes_history.csv"));
}

void HistoryManager::recordSalle(const Salle& salle, qint64 timestampMs)
{
    if (salle.id.trimmed().isEmpty())
        return;
    if (timestampMs <= 0)
        timestampMs = QDateTime::currentMSecsSinceEpoch();

    const qint64 bucket = timestampMs / kMinuteMs;
    m_latest.insert(salle.id, salle);

    PendingSample& pending = m_pending[salle.id];
    if (pending.bucket >= 0 && pending.bucket != bucket)
        if (!flushPending(salle.id))
            return;

    PendingSample& current = m_pending[salle.id];
    current.bucket = bucket;
    current.lastTimestampMs = timestampMs;
    ++current.count;
    if (salle.occupation >= 0) {
        current.occupationSum += salle.occupation;
        ++current.occupationValidCount;
        current.occupation = salle.occupation;
    }
    if (salle.densite >= 0.0 && qIsFinite(salle.densite)) {
        current.densiteSum += salle.densite;
        ++current.densiteValidCount;
    }
    if (!salle.fluxSortieHist.isEmpty()
        && qIsFinite(salle.fluxSortieHist.last())) {
        current.fluxSortieSum += salle.fluxSortieHist.last();
        ++current.fluxSortieValidCount;
        current.fluxSortie = salle.fluxSortieHist.last();
    }
    if (qIsFinite(salle.tendance)) {
        current.tendanceSum += salle.tendance;
        ++current.tendanceValidCount;
        current.tendance = salle.tendance;
    }
    if (salle.confiance >= 0.0 && qIsFinite(salle.confiance)) {
        current.confianceSum += salle.confiance;
        ++current.confianceValidCount;
        current.confiance = salle.confiance;
    }
    current.entrees = qMax(0, salle.nbEntrees);
    current.sorties = qMax(0, salle.nbSorties);
    current.regime = salle.regime;
}

bool HistoryManager::appendSample(const QString& salleId, const HistorySample& sample)
{
    ensureDirectory();
    const QString path = samplesPath(salleId);
    QFileInfo info(path);
    const bool writeHeader = !info.exists() || info.size() == 0;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        emit storageError(QStringLiteral("Impossible d'écrire l'historique de %1 : %2")
                              .arg(salleId, file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    if (writeHeader) {
        stream << csvRow({QStringLiteral("timestamp_iso"), QStringLiteral("timestamp_ms"),
                          QStringLiteral("occupation"), QStringLiteral("densite"),
                          QStringLiteral("entrees_total"), QStringLiteral("sorties_total"),
                          QStringLiteral("flux_sortie"), QStringLiteral("tendance"),
                          QStringLiteral("confiance"), QStringLiteral("regime"),
                          QStringLiteral("occupation_valid"), QStringLiteral("densite_valid"),
                          QStringLiteral("confiance_valid"), QStringLiteral("observations")});
    }
    stream << csvRow({timestampIso(sample.timestampMs), QString::number(sample.timestampMs),
                      QString::number(sample.occupation), number(sample.densite),
                      QString::number(sample.entrees), QString::number(sample.sorties),
                      number(sample.fluxSortie), number(sample.tendance),
                      number(sample.confiance), sample.regime,
                      sample.occupationValid ? QStringLiteral("1") : QStringLiteral("0"),
                      sample.densiteValid ? QStringLiteral("1") : QStringLiteral("0"),
                      sample.confianceValid ? QStringLiteral("1") : QStringLiteral("0"),
                      QString::number(sample.observations)});
    stream.flush();
    if (stream.status() != QTextStream::Ok) {
        emit storageError(QStringLiteral("Erreur d'écriture de l'historique de %1")
                              .arg(salleId));
        return false;
    }
    invalidateSamplesCache(salleId);
    return true;
}

bool HistoryManager::flushPending(const QString& salleId)
{
    if (!m_pending.contains(salleId))
        return true;

    const PendingSample pending = m_pending.take(salleId);
    if (pending.count <= 0)
        return true;

    HistorySample sample;
    sample.timestampMs = pending.lastTimestampMs;
    sample.occupationValid = pending.occupationValidCount > 0;
    sample.occupation = sample.occupationValid
                            ? qRound(pending.occupationSum / pending.occupationValidCount)
                            : -1;
    sample.densiteValid = pending.densiteValidCount > 0;
    sample.densite = sample.densiteValid
                         ? pending.densiteSum / pending.densiteValidCount
                         : -1.0;
    sample.entrees = pending.entrees;
    sample.sorties = pending.sorties;
    sample.fluxSortie = pending.fluxSortieValidCount > 0
                            ? pending.fluxSortieSum / pending.fluxSortieValidCount
                            : 0.0;
    sample.tendance = pending.tendanceValidCount > 0
                          ? pending.tendanceSum / pending.tendanceValidCount
                          : 0.0;
    sample.confianceValid = pending.confianceValidCount > 0;
    sample.confiance = sample.confianceValid
                           ? pending.confianceSum / pending.confianceValidCount
                           : -1.0;
    sample.observations = pending.count;
    sample.regime = pending.regime;
    if (!appendSample(salleId, sample)) {
        m_pending.insert(salleId, pending);
        return false;
    }
    return true;
}

void HistoryManager::flushCompleted(qint64 currentBucket)
{
    const QStringList ids = m_pending.keys();
    for (const QString& id : ids) {
        if (m_pending.value(id).bucket < currentBucket)
            flushPending(id);
    }
}

void HistoryManager::recordPassage(const QString& salleId, const QString& direction,
                                    qint64 timestampMs, const Salle& salle)
{
    if (salleId.trimmed().isEmpty()
        || (direction != QStringLiteral("entree")
            && direction != QStringLiteral("sortie"))) {
        return;
    }
    if (timestampMs <= 0)
        timestampMs = QDateTime::currentMSecsSinceEpoch();

    PassageEvent passage;
    passage.timestampMs = timestampMs;
    passage.salleId = salleId;
    passage.direction = direction;
    passage.occupation = salle.occupation;
    passage.entrees = salle.nbEntrees;
    passage.sorties = salle.nbSorties;
    m_pendingPassages[salleId].append(passage);
    if (m_pendingPassages.value(salleId).size() >= 64)
        flushPassages();
}

bool HistoryManager::appendPassages(const QString& salleId,
                                    const QVector<PassageEvent>& values)
{
    if (values.isEmpty())
        return true;
    ensureDirectory();
    const QString path = passagesPath(salleId);
    QFileInfo info(path);
    const bool writeHeader = !info.exists() || info.size() == 0;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        emit storageError(QStringLiteral("Impossible d'écrire les passages de %1 : %2")
                              .arg(salleId, file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    if (writeHeader) {
        stream << csvRow({QStringLiteral("timestamp_iso"), QStringLiteral("timestamp_ms"),
                          QStringLiteral("salle_id"), QStringLiteral("direction"),
                          QStringLiteral("occupation"), QStringLiteral("entrees_total"),
                          QStringLiteral("sorties_total")});
    }
    for (const PassageEvent& passage : values) {
        stream << csvRow({timestampIso(passage.timestampMs),
                          QString::number(passage.timestampMs), passage.salleId,
                          passage.direction, QString::number(passage.occupation),
                          QString::number(passage.entrees),
                          QString::number(passage.sorties)});
    }
    stream.flush();
    if (stream.status() != QTextStream::Ok) {
        emit storageError(QStringLiteral("Erreur d'écriture des passages de %1")
                              .arg(salleId));
        return false;
    }
    invalidatePassagesCache(salleId);
    return true;
}

void HistoryManager::flushPassages()
{
    const QStringList ids = m_pendingPassages.keys();
    for (const QString& id : ids) {
        const QVector<PassageEvent> values = m_pendingPassages.take(id);
        if (!appendPassages(id, values))
            m_pendingPassages[id] = values;
    }
}

void HistoryManager::recordAlerte(const Alerte& alerte)
{
    if (alerte.salleId.trimmed().isEmpty())
        return;

    Alerte value = alerte;
    if (value.ts == 0)
        value.ts = QDateTime::currentMSecsSinceEpoch();
    while (m_alertes.contains(value.ts)
           && m_alertes.value(value.ts).salleId != value.salleId) {
        ++value.ts;
    }
    m_alertes.insert(value.ts, value);
    QString error;
    if (!saveAlertes(&error) && !error.isEmpty())
        emit storageError(QStringLiteral("Impossible de sauvegarder les alertes : %1")
                              .arg(error));
}

void HistoryManager::updateAlerte(const Alerte& alerte)
{
    if (alerte.ts == 0 || !m_alertes.contains(alerte.ts))
        return;
    m_alertes.insert(alerte.ts, alerte);
    QString error;
    if (!saveAlertes(&error) && !error.isEmpty())
        emit storageError(QStringLiteral("Impossible de sauvegarder les alertes : %1")
                              .arg(error));
}

QList<Alerte> HistoryManager::alertes() const
{
    QList<Alerte> result = m_alertes.values();
    std::sort(result.begin(), result.end(), [](const Alerte& left, const Alerte& right) {
        return left.ts < right.ts;
    });
    return result;
}

void HistoryManager::loadAlertes()
{
    QFile file(alertesPath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    if (!stream.atEnd())
        stream.readLine();

    while (!stream.atEnd()) {
        const QStringList fields = csvFields(stream.readLine());
        if (fields.size() < 12)
            continue;
        Alerte value;
        value.ts = quint64(toInt64(fields.at(0)));
        value.salleId = fields.at(2);
        value.salleNom = fields.at(3);
        value.type = fields.at(4);
        value.detail = fields.at(5);
        value.score = toDouble(fields.at(6));
        value.capteurs = fields.at(7).split('|', Qt::SkipEmptyParts);
        value.appelCible = fields.at(8);
        value.appelStatut = fields.at(9);
        value.appelHeure = fields.at(10);
        value.acquittee = fields.at(11) == QStringLiteral("1");
        if (value.ts != 0)
            m_alertes.insert(value.ts, value);
    }
}

bool HistoryManager::saveAlertes(QString* error) const
{
    const_cast<HistoryManager*>(this)->ensureDirectory();
    QSaveFile file(alertesPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << csvRow({QStringLiteral("timestamp_ms"), QStringLiteral("timestamp_iso"),
                      QStringLiteral("salle_id"), QStringLiteral("salle_nom"),
                      QStringLiteral("type"), QStringLiteral("detail"),
                      QStringLiteral("score"), QStringLiteral("capteurs"),
                      QStringLiteral("appel_cible"), QStringLiteral("appel_statut"),
                      QStringLiteral("appel_heure"), QStringLiteral("acquittee")});
    for (const Alerte& value : alertes()) {
        stream << csvRow({QString::number(value.ts), timestampIso(qint64(value.ts)),
                          value.salleId, value.salleNom, value.type, value.detail,
                          number(value.score), value.capteurs.join('|'), value.appelCible,
                          value.appelStatut, value.appelHeure,
                          value.acquittee ? QStringLiteral("1") : QStringLiteral("0")});
    }

    stream.flush();
    if (stream.status() != QTextStream::Ok) {
        if (error)
            *error = QStringLiteral("Erreur d'écriture du fichier d'alertes.");
        return false;
    }

    if (!file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

qint64 HistoryManager::debutPeriode(HistoryPeriod period, qint64 referenceMs)
{
    const QDateTime reference = referenceMs > 0
                                    ? QDateTime::fromMSecsSinceEpoch(referenceMs)
                                    : QDateTime::currentDateTime();
    QDate date = reference.date();
    if (period == HistoryPeriod::Day)
        return QDateTime(date, QTime(0, 0), QTimeZone::systemTimeZone())
            .toMSecsSinceEpoch();
    if (period == HistoryPeriod::Week) {
        const int daysSinceMonday = date.dayOfWeek() - 1;
        date = date.addDays(-daysSinceMonday);
        return QDateTime(date, QTime(0, 0), QTimeZone::systemTimeZone())
            .toMSecsSinceEpoch();
    }
    date = QDate(date.year(), date.month(), 1);
    return QDateTime(date, QTime(0, 0), QTimeZone::systemTimeZone())
        .toMSecsSinceEpoch();
}

QString HistoryManager::libellePeriode(HistoryPeriod period)
{
    switch (period) {
    case HistoryPeriod::Day: return QStringLiteral("Jour");
    case HistoryPeriod::Week: return QStringLiteral("Semaine");
    case HistoryPeriod::Month: return QStringLiteral("Mois");
    }
    return QStringLiteral("Période");
}

QVector<HistorySample> HistoryManager::samples(const QString& salleId,
                                               HistoryPeriod period,
                                               qint64 referenceMs) const
{
    QVector<HistorySample> result = loadSamplesFromDisk(salleId);
    const qint64 endMs = referenceMs > 0 ? referenceMs
                                         : QDateTime::currentMSecsSinceEpoch();
    const qint64 startMs = debutPeriode(period, referenceMs);
    QVector<HistorySample> filtered;
    filtered.reserve(result.size() + 1);
    for (const HistorySample& sample : std::as_const(result)) {
        if (sample.timestampMs >= startMs && sample.timestampMs <= endMs)
            filtered.append(sample);
    }
    result = std::move(filtered);

    if (m_pending.contains(salleId)) {
        const PendingSample& pending = m_pending.value(salleId);
        const qint64 end = referenceMs > 0 ? referenceMs
                                           : QDateTime::currentMSecsSinceEpoch();
        if (pending.count > 0 && pending.lastTimestampMs >= startMs
            && pending.lastTimestampMs <= end) {
            HistorySample sample;
            sample.timestampMs = pending.lastTimestampMs;
            sample.occupationValid = pending.occupationValidCount > 0;
            sample.occupation = sample.occupationValid
                                    ? qRound(pending.occupationSum
                                             / pending.occupationValidCount)
                                    : -1;
            sample.densiteValid = pending.densiteValidCount > 0;
            sample.densite = sample.densiteValid
                                 ? pending.densiteSum / pending.densiteValidCount
                                 : -1.0;
            sample.entrees = pending.entrees;
            sample.sorties = pending.sorties;
            sample.fluxSortie = pending.fluxSortieValidCount > 0
                                    ? pending.fluxSortieSum / pending.fluxSortieValidCount
                                    : 0.0;
            sample.tendance = pending.tendanceValidCount > 0
                                  ? pending.tendanceSum / pending.tendanceValidCount
                                  : 0.0;
            sample.confianceValid = pending.confianceValidCount > 0;
            sample.confiance = sample.confianceValid
                                   ? pending.confianceSum / pending.confianceValidCount
                                   : -1.0;
            sample.observations = pending.count;
            sample.regime = pending.regime;
            result.append(sample);
        }
    }

    std::sort(result.begin(), result.end(), [](const HistorySample& left,
                                               const HistorySample& right) {
        return left.timestampMs < right.timestampMs;
    });
    return result;
}

QVector<PassageEvent> HistoryManager::passages(const QString& salleId,
                                               HistoryPeriod period,
                                               qint64 referenceMs) const
{
    QVector<PassageEvent> result = loadPassagesFromDisk(salleId);
    const qint64 endMs = referenceMs > 0 ? referenceMs
                                         : QDateTime::currentMSecsSinceEpoch();
    const qint64 startMs = debutPeriode(period, referenceMs);
    QVector<PassageEvent> filtered;
    filtered.reserve(result.size() + m_pendingPassages.value(salleId).size());
    for (const PassageEvent& passage : std::as_const(result)) {
        if (passage.timestampMs >= startMs && passage.timestampMs <= endMs)
            filtered.append(passage);
    }
    result = std::move(filtered);

    for (const PassageEvent& passage : m_pendingPassages.value(salleId)) {
        if (passage.timestampMs >= startMs && passage.timestampMs <= endMs)
            result.append(passage);
    }
    std::sort(result.begin(), result.end(), [](const PassageEvent& left,
                                               const PassageEvent& right) {
        return left.timestampMs < right.timestampMs;
    });
    return result;
}

QVector<HistorySample> HistoryManager::loadSamplesFromDisk(const QString& salleId) const
{
    const QString path = samplesPath(salleId);
    const QFileInfo info(path);
    CacheStamp stamp;
    stamp.modifiedMs = info.exists() ? info.lastModified().toMSecsSinceEpoch() : -1;
    stamp.size = info.exists() ? info.size() : -1;

    if (m_samplesCacheStamp.value(salleId).modifiedMs == stamp.modifiedMs
        && m_samplesCacheStamp.value(salleId).size == stamp.size
        && m_samplesCache.contains(salleId)) {
        return m_samplesCache.value(salleId);
    }

    QVector<HistorySample> result;
    QFile file(path);
    if (info.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        if (!stream.atEnd())
            stream.readLine();

        while (!stream.atEnd()) {
            const QStringList fields = csvFields(stream.readLine());
            if (fields.size() < 10)
                continue;

            bool timestampOk = false;
            const qint64 timestampMs = fields.at(1).toLongLong(&timestampOk);
            if (!timestampOk || timestampMs <= 0)
                continue;

            HistorySample sample;
            sample.timestampMs = timestampMs;
            sample.occupation = toInt(fields.at(2), -1);
            sample.densite = toDouble(fields.at(3), -1.0);
            sample.entrees = qMax(0, toInt(fields.at(4)));
            sample.sorties = qMax(0, toInt(fields.at(5)));
            sample.fluxSortie = qMax(0.0, toDouble(fields.at(6)));
            sample.tendance = toDouble(fields.at(7));
            sample.confiance = toDouble(fields.at(8), -1.0);
            sample.regime = fields.at(9);
            sample.occupationValid = fields.size() > 10
                                     ? fields.at(10) == QStringLiteral("1")
                                     : sample.occupation >= 0;
            sample.densiteValid = fields.size() > 11
                                  ? fields.at(11) == QStringLiteral("1")
                                  : sample.densite >= 0.0;
            sample.confianceValid = fields.size() > 12
                                    ? fields.at(12) == QStringLiteral("1")
                                    : sample.confiance >= 0.0;
            sample.observations = fields.size() > 13
                                  ? qMax(1, toInt(fields.at(13), 1))
                                  : 1;
            result.append(sample);
        }
    }

    m_samplesCache.insert(salleId, result);
    m_samplesCacheStamp.insert(salleId, stamp);
    return result;
}

QVector<PassageEvent> HistoryManager::loadPassagesFromDisk(const QString& salleId) const
{
    const QString path = passagesPath(salleId);
    const QFileInfo info(path);
    CacheStamp stamp;
    stamp.modifiedMs = info.exists() ? info.lastModified().toMSecsSinceEpoch() : -1;
    stamp.size = info.exists() ? info.size() : -1;

    if (m_passagesCacheStamp.value(salleId).modifiedMs == stamp.modifiedMs
        && m_passagesCacheStamp.value(salleId).size == stamp.size
        && m_passagesCache.contains(salleId)) {
        return m_passagesCache.value(salleId);
    }

    QVector<PassageEvent> result;
    QFile file(path);
    if (info.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        if (!stream.atEnd())
            stream.readLine();

        while (!stream.atEnd()) {
            const QStringList fields = csvFields(stream.readLine());
            if (fields.size() < 7)
                continue;
            bool timestampOk = false;
            const qint64 timestampMs = fields.at(1).toLongLong(&timestampOk);
            if (!timestampOk || timestampMs <= 0)
                continue;

            PassageEvent passage;
            passage.timestampMs = timestampMs;
            passage.salleId = fields.at(2);
            passage.direction = fields.at(3);
            if (passage.direction != QStringLiteral("entree")
                && passage.direction != QStringLiteral("sortie")) {
                continue;
            }
            passage.occupation = toInt(fields.at(4), -1);
            passage.entrees = qMax(0, toInt(fields.at(5)));
            passage.sorties = qMax(0, toInt(fields.at(6)));
            result.append(passage);
        }
    }

    m_passagesCache.insert(salleId, result);
    m_passagesCacheStamp.insert(salleId, stamp);
    return result;
}

void HistoryManager::invalidateSamplesCache(const QString& salleId)
{
    m_samplesCache.remove(salleId);
    m_samplesCacheStamp.remove(salleId);
}

void HistoryManager::invalidatePassagesCache(const QString& salleId)
{
    m_passagesCache.remove(salleId);
    m_passagesCacheStamp.remove(salleId);
}

QList<Alerte> HistoryManager::alertes(const QString& salleId,
                                      HistoryPeriod period,
                                      qint64 referenceMs) const
{
    QList<Alerte> result;
    for (const Alerte& value : m_alertes) {
        if (value.salleId == salleId && inPeriod(qint64(value.ts), period, referenceMs))
            result.append(value);
    }
    std::sort(result.begin(), result.end(), [](const Alerte& left, const Alerte& right) {
        return left.ts < right.ts;
    });
    return result;
}

HistoryStats HistoryManager::analyse(const QString& salleId,
                                     HistoryPeriod period,
                                     qint64 referenceMs) const
{
    HistoryStats result;
    const QVector<HistorySample> values = samples(salleId, period, referenceMs);
    const QVector<PassageEvent> events = passages(salleId, period, referenceMs);
    result.nombrePoints = values.size();
    if (!values.isEmpty()) {
        int occupationCount = 0;
        int densiteCount = 0;
        for (const HistorySample& value : values) {
            if (value.occupationValid && value.occupation >= 0) {
                result.occupationMoyenne += value.occupation;
                ++occupationCount;
                if (!result.aUnPic || value.occupation > result.pic.occupation)
                    result.pic = value;
                if (!result.aUnCreux || value.occupation < result.creux.occupation)
                    result.creux = value;
                result.aUnPic = true;
                result.aUnCreux = true;
            }
            if (value.densiteValid && value.densite >= 0.0) {
                result.densiteMoyenne += value.densite;
                ++densiteCount;
            }
        }
        if (occupationCount > 0)
            result.occupationMoyenne /= double(occupationCount);
        if (densiteCount > 0)
            result.densiteMoyenne /= double(densiteCount);
    }
    for (const PassageEvent& event : events) {
        if (event.direction == QStringLiteral("entree"))
            ++result.nombreEntrees;
        else if (event.direction == QStringLiteral("sortie"))
            ++result.nombreSorties;
    }
    return result;
}

bool HistoryManager::exportSalleCsv(const QString& salleId, HistoryPeriod period,
                                    const QString& filePath, QString* error) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << csvRow({QStringLiteral("type"), QStringLiteral("timestamp_iso"),
                      QStringLiteral("timestamp_ms"), QStringLiteral("direction"),
                      QStringLiteral("occupation"), QStringLiteral("densite"),
                      QStringLiteral("entrees_total"), QStringLiteral("sorties_total"),
                      QStringLiteral("flux_sortie"), QStringLiteral("tendance"),
                      QStringLiteral("confiance"), QStringLiteral("regime"),
                      QStringLiteral("detail")});

    const QVector<HistorySample> values = samples(salleId, period);
    for (const HistorySample& value : values) {
        stream << csvRow({QStringLiteral("echantillon"), timestampIso(value.timestampMs),
                          QString::number(value.timestampMs), QString(),
                          QString::number(value.occupation), number(value.densite),
                          QString::number(value.entrees), QString::number(value.sorties),
                          number(value.fluxSortie), number(value.tendance),
                          number(value.confiance), value.regime, QString()});
    }

    const QVector<PassageEvent> events = passages(salleId, period);
    for (const PassageEvent& event : events) {
        stream << csvRow({QStringLiteral("passage"), timestampIso(event.timestampMs),
                          QString::number(event.timestampMs), event.direction,
                          QString::number(event.occupation), QString(),
                          QString::number(event.entrees), QString::number(event.sorties),
                          QString(), QString(), QString(), QString(), QString()});
    }

    const QList<Alerte> roomAlerts = alertes(salleId, period);
    for (const Alerte& alert : roomAlerts) {
        stream << csvRow({QStringLiteral("alerte"), timestampIso(qint64(alert.ts)),
                          QString::number(alert.ts), QString(), QString(), QString(),
                          QString(), QString(), QString(), QString(), QString(),
                          QString(), alert.detail});
    }

    if (stream.status() != QTextStream::Ok) {
        if (error)
            *error = QStringLiteral("Erreur d'écriture du fichier CSV.");
        return false;
    }
    return true;
}

bool HistoryManager::exportAlertesCsv(const QString& filePath,
                                      const QList<Alerte>& values,
                                      QString* error) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << csvRow({QStringLiteral("timestamp_iso"), QStringLiteral("timestamp_ms"),
                      QStringLiteral("salle_id"), QStringLiteral("salle_nom"),
                      QStringLiteral("type"), QStringLiteral("detail"),
                      QStringLiteral("score"), QStringLiteral("capteurs"),
                      QStringLiteral("appel_cible"), QStringLiteral("appel_statut"),
                      QStringLiteral("appel_heure"), QStringLiteral("acquittee")});
    for (const Alerte& value : values) {
        stream << csvRow({timestampIso(qint64(value.ts)), QString::number(value.ts),
                          value.salleId, value.salleNom, value.type, value.detail,
                          number(value.score), value.capteurs.join('|'), value.appelCible,
                          value.appelStatut, value.appelHeure,
                          value.acquittee ? QStringLiteral("1") : QStringLiteral("0")});
    }
    if (stream.status() != QTextStream::Ok) {
        if (error)
            *error = QStringLiteral("Erreur d'écriture du fichier CSV.");
        return false;
    }
    return true;
}

void HistoryManager::resetLive()
{
    m_latest.clear();
    flush();
}

void HistoryManager::flush()
{
    const QStringList ids = m_pending.keys();
    for (const QString& id : ids)
        flushPending(id);
    flushPassages();
}

void HistoryManager::onMinute()
{
    const qint64 timestampMs = QDateTime::currentMSecsSinceEpoch();
    flushCompleted(timestampMs / kMinuteMs);
    for (const Salle& salle : m_latest.values())
        recordSalle(salle, timestampMs);
    flushPassages();
}
