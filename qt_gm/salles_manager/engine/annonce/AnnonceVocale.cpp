#include "engine/annonce/AnnonceVocale.h"

#include <QStandardPaths>

AnnonceVocale::AnnonceVocale(QObject* parent)
    : QObject(parent)
{
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int, QProcess::ExitStatus) { emit termine(); });

    const QString espeakNg = QStandardPaths::findExecutable(QStringLiteral("espeak-ng"));
    if (!espeakNg.isEmpty()) {
        m_backend = QStringLiteral("espeak-ng");
        m_disponible = true;
        return;
    }
    const QString espeak = QStandardPaths::findExecutable(QStringLiteral("espeak"));
    if (!espeak.isEmpty()) {
        m_backend = QStringLiteral("espeak");
        m_disponible = true;
        return;
    }
    const QString spdSay = QStandardPaths::findExecutable(QStringLiteral("spd-say"));
    if (!spdSay.isEmpty()) {
        m_backend = QStringLiteral("spd-say");
        m_disponible = true;
        return;
    }
    m_backend = QString();
    m_disponible = false;
}

void AnnonceVocale::parler(const QString& texte, const QString& langue)
{
    if (!m_disponible || texte.trimmed().isEmpty())
        return;

    arreter();

    const QString voix = (langue == QStringLiteral("en"))
                             ? QStringLiteral("en")
                             : QStringLiteral("fr");
    if (m_backend == QStringLiteral("spd-say")) {
        m_process.start(QStringLiteral("spd-say"),
                        { QStringLiteral("-l"), voix, texte });
    } else {
        m_process.start(m_backend,
                        { QStringLiteral("-v"), voix,
                          QStringLiteral("-s"), QStringLiteral("160"),
                          QStringLiteral("-a"), QStringLiteral("200"),
                          texte });
    }
}

void AnnonceVocale::arreter()
{
    if (m_process.state() == QProcess::NotRunning)
        return;
    if (m_backend == QStringLiteral("spd-say"))
        QProcess::startDetached(QStringLiteral("spd-say"),
                                { QStringLiteral("--cancel") });
    m_process.kill();
    m_process.waitForFinished(200);
}