#pragma once

#include <QObject>
#include <QString>
#include <QMutex>

class TtsWav : public QObject
{
    Q_OBJECT
public:
    explicit TtsWav(QObject* parent = nullptr);
    ~TtsWav() override;

    // Génère un fichier WAV via libespeak-ng
    // Retourne le chemin du fichier généré (dans /tmp/opencode/tts/) ou chaîne vide si échec
    // Le fichier est au format WAV 16-bit mono 22050 Hz (compatible Asterisk format_wav)
    QString generer(const QString& texte, const QString& langue = "fr");

    // Nettoie les fichiers temporaires de plus de 1 heure
    void nettoyerAncien();

signals:
    void wavGenere(const QString& cheminFichier);
    void erreur(const QString& message);

private:
    struct EspeakDeleter {
        void operator()(void* handle);
    };

    bool initialiserEspeak(const QString& langue);
    void nettoyerEspeak();
    QString cheminCache() const;

    void* m_espeakHandle = nullptr;
    QString m_langueActuelle;
    QMutex m_mutex;
    QString m_repertoireCache;
};