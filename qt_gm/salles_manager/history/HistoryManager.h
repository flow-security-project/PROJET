#pragma once

#include <QHash>
#include <QObject>
#include <QVector>

#include "models/Alerte.h"
#include "models/Salle.h"

enum class HistoryPeriod {
    Day,
    Week,
    Month
};

struct HistorySample
{
    qint64 timestampMs = 0;
    int occupation = 0;
    bool occupationValid = true;
    double densite = 0.0;
    bool densiteValid = true;
    int entrees = 0;
    int sorties = 0;
    double fluxSortie = 0.0;
    double tendance = 0.0;
    double confiance = -1.0;
    bool confianceValid = false;
    QString regime;
    int observations = 1;
};

struct PassageEvent
{
    qint64 timestampMs = 0;
    QString salleId;
    QString direction;
    int occupation = -1;
    int entrees = 0;
    int sorties = 0;
};

struct HistoryStats
{
    int nombrePoints = 0;
    double occupationMoyenne = 0.0;
    double densiteMoyenne = 0.0;
    bool aUnPic = false;
    bool aUnCreux = false;
    HistorySample pic;
    HistorySample creux;
    int nombreEntrees = 0;
    int nombreSorties = 0;
};

class HistoryManager : public QObject
{
    Q_OBJECT

public:
    explicit HistoryManager(const QString& directory = QString(),
                            QObject* parent = nullptr);
    ~HistoryManager() override;

    QString directory() const { return m_directory; }

    void recordSalle(const Salle& salle, qint64 timestampMs = 0);
    void recordPassage(const QString& salleId, const QString& direction,
                       qint64 timestampMs, const Salle& salle);

    void recordAlerte(const Alerte& alerte);
    void updateAlerte(const Alerte& alerte);
    QList<Alerte> alertes() const;

    QVector<HistorySample> samples(const QString& salleId,
                                   HistoryPeriod period,
                                   qint64 referenceMs = 0) const;
    QVector<PassageEvent> passages(const QString& salleId,
                                   HistoryPeriod period,
                                   qint64 referenceMs = 0) const;
    QList<Alerte> alertes(const QString& salleId,
                          HistoryPeriod period,
                          qint64 referenceMs = 0) const;
    HistoryStats analyse(const QString& salleId,
                         HistoryPeriod period,
                         qint64 referenceMs = 0) const;

    bool exportSalleCsv(const QString& salleId, HistoryPeriod period,
                        const QString& filePath, QString* error = nullptr) const;
    bool exportAlertesCsv(const QString& filePath, const QList<Alerte>& alertes,
                          QString* error = nullptr) const;

    static qint64 debutPeriode(HistoryPeriod period, qint64 referenceMs = 0);
    static QString libellePeriode(HistoryPeriod period);

    void resetLive();
    void flush();

signals:
    void storageError(const QString& message);

private slots:
    void onMinute();

private:
    struct PendingSample {
        qint64 bucket = -1;
        qint64 lastTimestampMs = 0;
        int count = 0;
        double occupationSum = 0.0;
        int occupationValidCount = 0;
        double densiteSum = 0.0;
        int densiteValidCount = 0;
        double fluxSortieSum = 0.0;
        int fluxSortieValidCount = 0;
        double tendanceSum = 0.0;
        int tendanceValidCount = 0;
        double confianceSum = 0.0;
        int confianceValidCount = 0;
        int occupation = 0;
        int entrees = 0;
        int sorties = 0;
        double fluxSortie = 0.0;
        double tendance = 0.0;
        double confiance = -1.0;
        QString regime;
    };

    struct CacheStamp {
        qint64 modifiedMs = -1;
        qint64 size = -1;
    };

    bool appendSample(const QString& salleId, const HistorySample& sample);
    bool flushPending(const QString& salleId);
    void flushCompleted(qint64 currentBucket);
    void flushPassages();
    bool appendPassages(const QString& salleId,
                        const QVector<PassageEvent>& passages);
    QVector<HistorySample> loadSamplesFromDisk(const QString& salleId) const;
    QVector<PassageEvent> loadPassagesFromDisk(const QString& salleId) const;
    void invalidateSamplesCache(const QString& salleId);
    void invalidatePassagesCache(const QString& salleId);
    void ensureDirectory();
    void loadAlertes();
    bool saveAlertes(QString* error = nullptr) const;

    QString salleStem(const QString& salleId) const;
    QString samplesPath(const QString& salleId) const;
    QString passagesPath(const QString& salleId) const;
    QString alertesPath() const;

    QHash<QString, Salle> m_latest;
    QHash<QString, PendingSample> m_pending;
    QHash<QString, QVector<PassageEvent>> m_pendingPassages;
    QHash<quint64, Alerte> m_alertes;
    mutable QHash<QString, QVector<HistorySample>> m_samplesCache;
    mutable QHash<QString, CacheStamp> m_samplesCacheStamp;
    mutable QHash<QString, QVector<PassageEvent>> m_passagesCache;
    mutable QHash<QString, CacheStamp> m_passagesCacheStamp;
    QString m_directory;
    class QTimer* m_timer = nullptr;
};
