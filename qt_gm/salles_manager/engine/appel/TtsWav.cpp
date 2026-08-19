#include "TtsWav.h"

#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QStandardPaths>
#include <QDebug>

#if defined(HAVE_ESPEAK_NG) || defined(ESPEAK_NG_FOUND)
#include <espeak-ng/speak_lib.h>
#else
// Fallback: on définit les symboles nécessaires pour la compilation
#define EE_OK 0
#define AUDIO_OUTPUT_SYNCHRONOUS 0
#define POS_CHARACTER 0
#define espeakCHARS_UTF8 0
#define espeakRATE 0
#define espeakVOLUME 1
#define espeakPITCH 2
typedef int espeak_ERROR;
int espeak_Initialize(int, int, const char*, int) { return 0; }
int espeak_SetVoiceByName(const char*) { return EE_OK; }
int espeak_SetParameter(int, int, int) { return EE_OK; }
int espeak_SetWavFile(const char*) { return EE_OK; }
int espeak_Synth(const char*, size_t, size_t, int, int, int, void*, void*) { return EE_OK; }
int espeak_Terminate() { return EE_OK; }
#endif

constexpr int ESPEAK_SAMPLE_RATE = 22050;
constexpr int CACHE_MAX_AGE_MS = 3600000; // 1 heure

void TtsWav::EspeakDeleter::operator()(void* handle)
{
    if (handle) {
        espeak_Terminate();
    }
}

TtsWav::TtsWav(QObject* parent)
    : QObject(parent)
{
    m_repertoireCache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/tts";
    QDir().mkpath(m_repertoireCache);
}

TtsWav::~TtsWav()
{
    nettoyerEspeak();
}

bool TtsWav::initialiserEspeak(const QString& langue)
{
    QMutexLocker lock(&m_mutex);

    if (m_espeakHandle && m_langueActuelle == langue)
        return true;

    nettoyerEspeak();

    // Initialisation espeak-ng en mode fichier WAV (AUDIO_OUTPUT_SYNCHRONOUS)
    espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, nullptr, 0);

    // Sélection de la voix
    const char* voiceName = (langue == "en") ? "en" : "fr";
    if (espeak_SetVoiceByName(voiceName) != EE_OK) {
        qWarning() << "TtsWav: impossible de charger la voix" << voiceName;
        return false;
    }

    // Paramètres de qualité
    espeak_SetParameter(espeakRATE, 160, 0);     // vitesse (mots/min)
    espeak_SetParameter(espeakVOLUME, 100, 0);    // volume 0-100
    espeak_SetParameter(espeakPITCH, 50, 0);      // hauteur 0-99

    m_langueActuelle = langue;
    return true;
}

void TtsWav::nettoyerEspeak()
{
    QMutexLocker lock(&m_mutex);
    if (m_espeakHandle) {
        espeak_Terminate();
        m_espeakHandle = nullptr;
    }
    m_langueActuelle.clear();
}

QString TtsWav::cheminCache() const
{
    return m_repertoireCache;
}

QString TtsWav::generer(const QString& texte, const QString& langue)
{
    if (texte.trimmed().isEmpty())
        return QString();

    if (!initialiserEspeak(langue))
        return QString();

    // Fichier de sortie unique (timestamp + hash texte)
    const QString hash = QString::number(qHash(texte + langue), 16);
    const QString nomFichier = QString("tts_%1_%2.wav")
                                   .arg(QDateTime::currentMSecsSinceEpoch())
                                   .arg(hash);
    const QString cheminSortie = m_repertoireCache + "/" + nomFichier;

    // Configurer espeak pour écrire dans le fichier WAV
    QByteArray cheminBytes = cheminSortie.toLocal8Bit();
    if (espeak_SetWavFile(cheminBytes.constData()) != EE_OK) {
        qWarning() << "TtsWav: impossible d'ouvrir le fichier WAV pour écriture:" << cheminSortie;
        return QString();
    }

    // Synthèse synchrone (bloquante jusqu'à la fin)
    QByteArray texteBytes = texte.toUtf8();
    espeak_ERROR err = espeak_Synth(texteBytes.constData(),
                                    texteBytes.size() + 1,
                                    0,                  // position
                                    POS_CHARACTER,
                                    0,                  // end position
                                    espeakCHARS_UTF8,
                                    nullptr,            // callback (synchrone)
                                    nullptr);           // user data

    if (err != EE_OK) {
        qWarning() << "TtsWav: erreur de synthèse espeak:" << err;
        espeak_SetWavFile(nullptr);
        return QString();
    }

    // Fermer le fichier WAV
    espeak_SetWavFile(nullptr);

    // Vérifier que le fichier a été créé et a une taille raisonnable
    QFileInfo fi(cheminSortie);
    if (!fi.exists() || fi.size() < 100) {
        qWarning() << "TtsWav: fichier WAV non généré ou trop petit:" << cheminSortie;
        return QString();
    }

    emit wavGenere(cheminSortie);
    return cheminSortie;
}

void TtsWav::nettoyerAncien()
{
    QDir cache(m_repertoireCache);
    if (!cache.exists())
        return;

    const qint64 maintenant = QDateTime::currentMSecsSinceEpoch();
    for (const QFileInfo& fi : cache.entryInfoList(QDir::Files)) {
        if (maintenant - fi.lastModified().toMSecsSinceEpoch() > CACHE_MAX_AGE_MS) {
            cache.remove(fi.fileName());
        }
    }
}