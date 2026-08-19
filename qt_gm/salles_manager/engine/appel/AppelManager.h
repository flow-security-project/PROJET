#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QDateTime>

class DataSource;
struct Alerte;
struct Salle;

#include "engine/appel/AriClient.h"
#include "engine/appel/AmiClient.h"
#include "engine/appel/TtsWav.h"
#include "engine/appel/MessagesBilingues.h"

class AppelManager : public QObject
{
    Q_OBJECT
public:
    struct ConfigGlobale {
        bool actif = true;
        QString numeroDefaut;           // ex: "1001"
        AriClient::Config ari;
        AmiClient::Config ami;
    };

    struct ConfigSalle {
        QString numero;                 // vide = utiliser numeroDefaut
        bool appelIntrusion = true;
        bool appelBousculade = true;
        bool appelEvacuation = false;
    };

    explicit AppelManager(QObject* parent = nullptr);
    ~AppelManager() override;

    void setSource(DataSource* source);
    void setConfigGlobale(const ConfigGlobale& config);
    void setConfigSalle(const QString& salleId, const ConfigSalle& config);

    bool actif() const { return m_actif; }
    void setActif(bool actif);

    // Appel de test (bouton TEST APPEL)
    void testAppel(const QString& numero = QString(), const QString& langue = "fr");

signals:
    void statutAppelChange(const QString& salleId, const QString& channelId,
                           const QString& etat, const QString& extension);
    void erreur(const QString& message);
    void log(const QString& message);

private slots:
    void surAlerte(const Alerte& alerte);
    void surAppelChange(const AriClient::AppelInfo& info);
    void surAppelTermine(const QString& channelId, bool succes);
    void surMessageEnvoye(const QString& extension, bool succes, const QString& details);
    void surWavGenere(const QString& chemin);
    void surErreurAri(const QString& msg);
    void surErreurAmi(const QString& msg);
    void surConnexionAriChangee(bool connecte);
    void surConnexionAmiChangee(bool connecte);

private:
    void traiterAlerte(const Alerte& alerte);
    void declencherAppel(const Alerte& alerte);
    void envoyerMessageSip(const Alerte& alerte);
    QString determinerNumero(const Alerte& alerte) const;
    QString determinerLangue(const Alerte& alerte) const;
    AppelMessages::EventKey typeVersEventKey(const QString& type) const;
    bool verifierAntiSpam(const QString& cle) const;
    void marquerAntiSpam(const QString& cle);

    DataSource* m_source = nullptr;
    ConfigGlobale m_configGlobale;
    QHash<QString, ConfigSalle> m_configSalles;

    AriClient* m_ari = nullptr;
    AmiClient* m_ami = nullptr;
    TtsWav* m_tts = nullptr;

    bool m_actif = true;

    struct AntiSpam {
        QDateTime dernierAppel;
        int compteur = 0;
    };
    QHash<QString, AntiSpam> m_antiSpam; // clé: "salleId:type"
    static constexpr int ANTISPAM_DELAI_MS = 5 * 60 * 1000; // 5 min
    static constexpr int ANTISPAM_MAX_PAR_FENETRE = 3;
};