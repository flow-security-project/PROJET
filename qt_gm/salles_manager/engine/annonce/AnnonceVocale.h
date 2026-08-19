#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

// Backend de synthèse vocale (annonces du superviseur) :
//   1. espeak-ng (binaire) si disponible ;
//   2. sinon spd-say (Speech Dispatcher, démon utilisateur).
// La lecture est asynchrone : un QProcess par annonce, interrompable.
class AnnonceVocale : public QObject
{
    Q_OBJECT

public:
    explicit AnnonceVocale(QObject* parent = nullptr);

    bool disponible() const { return m_disponible; }
    QString backend() const { return m_backend; }

    void parler(const QString& texte, const QString& langue = QStringLiteral("fr"));
    void arreter();

signals:
    void termine();

private:
    QProcess m_process;
    QString m_backend;
    bool m_disponible = false;
};