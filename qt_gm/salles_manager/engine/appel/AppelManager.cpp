#include "AppelManager.h"

#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

#include "data/DataSource.h"
#include "models/Alerte.h"
#include "models/Salle.h"

AppelManager::AppelManager(QObject* parent)
    : QObject(parent)
{
    m_ari = new AriClient(AriClient::Config(), this);
    m_ami = new AmiClient(AmiClient::Config(), this);
    m_tts = new TtsWav(this);

    connect(m_ari, &AriClient::appelChange, this, &AppelManager::surAppelChange);
    connect(m_ari, &AriClient::appelTermine, this, &AppelManager::surAppelTermine);
    connect(m_ari, &AriClient::erreur, this, &AppelManager::surErreurAri);
    connect(m_ari, &AriClient::connexionChangee, this, &AppelManager::surConnexionAriChangee);

    connect(m_ami, &AmiClient::messageEnvoye, this, &AppelManager::surMessageEnvoye);
    connect(m_ami, &AmiClient::erreur, this, &AppelManager::surErreurAmi);
    connect(m_ami, &AmiClient::connexionChangee, this, &AppelManager::surConnexionAmiChangee);

    connect(m_tts, &TtsWav::wavGenere, this, &AppelManager::surWavGenere);
    connect(m_tts, &TtsWav::erreur, this, &AppelManager::surErreurAri);
}

AppelManager::~AppelManager()
{
}

void AppelManager::setSource(DataSource* source)
{
    if (m_source == source)
        return;
    if (m_source)
        disconnect(m_source, nullptr, this, nullptr);
    m_source = source;
    if (m_source) {
        connect(m_source, &DataSource::alerte, this, &AppelManager::surAlerte);
    }
}

void AppelManager::setConfigGlobale(const ConfigGlobale& config)
{
    m_configGlobale = config;
    m_ari->setConfig(config.ari);
    m_ami->setConfig(config.ami);
}

void AppelManager::setConfigSalle(const QString& salleId, const ConfigSalle& config)
{
    m_configSalles[salleId] = config;
}

void AppelManager::setActif(bool actif)
{
    m_actif = actif;
}

void AppelManager::testAppel(const QString& numero, const QString& langue)
{
    if (!m_actif) {
        emit log("Test appel ignoré : système inactif");
        return;
    }

    const QString num = numero.isEmpty() ? m_configGlobale.numeroDefaut : numero;
    if (num.isEmpty()) {
        emit erreur("Test appel : aucun numéro configuré (ni global, ni paramètre)");
        return;
    }

    if (!m_ari->estConnecte()) {
        emit erreur("Test appel : ARI non connecté");
        return;
    }

    const QString texte = AppelMessages::texte(AppelMessages::EventKey::TestVocale, langue);
    const QString wavName = QString("test_%1.wav").arg(QDateTime::currentMSecsSinceEpoch());
    const QString wavPath = m_tts->generer(texte, langue);

    if (wavPath.isEmpty()) {
        emit erreur("Test appel : échec génération WAV");
        return;
    }

    // Copier le WAV vers /var/lib/asterisk/sounds/ (nécessite droits)
    // Pour le test, on suppose que le fichier est accessible ou on utilise un son intégré
    // Ici on essaie de copier, sinon on utilise un son de démo
    QString wavDest = "/var/lib/asterisk/sounds/" + wavName;
    if (!QFile::copy(wavPath, wavDest)) {
        qWarning() << "Impossible de copier WAV vers" << wavDest << "- utilisation son démo";
        // Fallback : utiliser un son intégré d'Asterisk
        const QString channelId = m_ari->appeler(num, "hello-world");
        if (!channelId.isEmpty()) {
            emit log(QString("Test appel lancé vers %1 (son démo)").arg(num));
        }
        return;
    }

    const QString channelId = m_ari->appeler(num, wavName);
    if (!channelId.isEmpty()) {
        emit log(QString("Test appel lancé vers %1 (WAV généré)").arg(num));
    } else {
        emit erreur("Test appel : échec lancement appel ARI");
    }
}

void AppelManager::surAlerte(const Alerte& alerte)
{
    if (!m_actif)
        return;
    traiterAlerte(alerte);
}

void AppelManager::traiterAlerte(const Alerte& alerte)
{
    // Vérifier si cette alerte doit déclencher un appel
    const ConfigSalle config = m_configSalles.value(alerte.salleId);
    bool declencher = false;

    if (alerte.type == QStringLiteral("intrusion") && config.appelIntrusion)
        declencher = true;
    else if (alerte.type == QStringLiteral("bousculade") && config.appelBousculade)
        declencher = true;
    else if (alerte.type == QStringLiteral("evacuation") && config.appelEvacuation)
        declencher = true;

    if (!declencher)
        return;

    // Anti-spam
    const QString cle = QString("%1:%2").arg(alerte.salleId, alerte.type);
    if (!verifierAntiSpam(cle))
        return;

    marquerAntiSpam(cle);
    declencherAppel(alerte);
    envoyerMessageSip(alerte);
}

void AppelManager::declencherAppel(const Alerte& alerte)
{
    if (!m_ari->estConnecte()) {
        emit log(QString("Appel non lancé (ARI déconnecté): %1/%2").arg(alerte.salleId, alerte.type));
        return;
    }

    const QString numero = determinerNumero(alerte);
    if (numero.isEmpty()) {
        emit log(QString("Appel non lancé (pas de numéro): %1/%2").arg(alerte.salleId, alerte.type));
        return;
    }

    const QString langue = determinerLangue(alerte);
    const AppelMessages::EventKey key = typeVersEventKey(alerte.type);

    QString texteAppel;
    Salle salle;
    bool salleTrouvee = false;
    if (m_source && m_source->salles().contains(alerte.salleId)) {
        salle = m_source->salles().value(alerte.salleId);
        salleTrouvee = true;
    }

    const QString nomSalle = salleTrouvee ? (salle.nom.isEmpty() ? salle.id : salle.nom) : alerte.salleId;

    if (key == AppelMessages::EventKey::Attente && salleTrouvee) {
        texteAppel = AppelMessages::texteAttente(langue, nomSalle, salle.attenteEstimeeMin);
    } else if (key == AppelMessages::EventKey::Redirection && salleTrouvee && !salle.redirectionVers.isEmpty()) {
        texteAppel = AppelMessages::texteRedirection(langue, nomSalle, salle.redirectionVers);
    } else {
        texteAppel = AppelMessages::texte(key, langue, nomSalle);
    }

    // Générer WAV
    const QString wavName = QString("appel_%1_%2_%3.wav")
                                .arg(alerte.salleId)
                                .arg(alerte.type)
                                .arg(QDateTime::currentMSecsSinceEpoch());
    const QString wavPath = m_tts->generer(texteAppel, langue);

    if (wavPath.isEmpty()) {
        emit erreur(QString("Échec génération WAV pour appel: %1/%2").arg(alerte.salleId, alerte.type));
        return;
    }

    // Copier vers /var/lib/asterisk/sounds/
    const QString wavDest = QString("/var/lib/asterisk/sounds/%1").arg(wavName);
    if (!QFile::copy(wavPath, wavDest)) {
        qWarning() << "Impossible de copier WAV vers" << wavDest;
        emit erreur(QString("Impossible de copier WAV pour appel: %1/%2").arg(alerte.salleId, alerte.type));
        return;
    }

    // Lancer l'appel
    const QString channelId = m_ari->appeler(numero, wavName);
    if (channelId.isEmpty()) {
        emit erreur(QString("Échec lancement appel ARI: %1/%2").arg(alerte.salleId, alerte.type));
        return;
    }

    emit log(QString("Appel lancé: %1 → %2 (%3)").arg(alerte.type, nomSalle, numero));
}

void AppelManager::envoyerMessageSip(const Alerte& alerte)
{
    if (!m_ami->estConnecte()) {
        emit log(QString("Message SIP non envoyé (AMI déconnecté): %1/%2").arg(alerte.salleId, alerte.type));
        return;
    }

    const QString numero = determinerNumero(alerte);
    if (numero.isEmpty())
        return;

    const QString langue = determinerLangue(alerte);
    const AppelMessages::EventKey key = typeVersEventKey(alerte.type);

    QString texteMessage;
    Salle salle;
    bool salleTrouvee = false;
    if (m_source && m_source->salles().contains(alerte.salleId)) {
        salle = m_source->salles().value(alerte.salleId);
        salleTrouvee = true;
    }

    const QString nomSalle = salleTrouvee ? (salle.nom.isEmpty() ? salle.id : salle.nom) : alerte.salleId;

    if (key == AppelMessages::EventKey::Attente && salleTrouvee) {
        texteMessage = AppelMessages::texteAttente(langue, nomSalle, salle.attenteEstimeeMin);
    } else if (key == AppelMessages::EventKey::Redirection && salleTrouvee && !salle.redirectionVers.isEmpty()) {
        texteMessage = AppelMessages::texteRedirection(langue, nomSalle, salle.redirectionVers);
    } else {
        texteMessage = AppelMessages::texte(key, langue, nomSalle);
    }

    // Préfixe pour identifier l'alerte
    QString prefix;
    if (langue == "en")
        prefix = QString("[i++] %1 ALERT: ").arg(alerte.type.toUpper());
    else
        prefix = QString("[i++] ALERTE %1: ").arg(alerte.type.toUpper());

    const QString messageComplet = prefix + texteMessage;

    if (m_ami->envoyerMessage(numero, messageComplet)) {
        emit log(QString("Message SIP envoyé à %1: %2").arg(numero, alerte.type));
    }
}

QString AppelManager::determinerNumero(const Alerte& alerte) const
{
    const ConfigSalle config = m_configSalles.value(alerte.salleId);
    if (!config.numero.isEmpty())
        return config.numero;
    return m_configGlobale.numeroDefaut;
}

QString AppelManager::determinerLangue(const Alerte& alerte) const
{
    if (m_source && m_source->salles().contains(alerte.salleId)) {
        const Salle& s = m_source->salles().value(alerte.salleId);
        if (s.langue == "en" || s.langue == "fr")
            return s.langue;
    }
    // Fallback : langue de l'appel global (à ajouter si nécessaire)
    return "fr";
}

AppelMessages::EventKey AppelManager::typeVersEventKey(const QString& type) const
{
    if (type == QStringLiteral("evacuation"))       return AppelMessages::EventKey::Evacuation;
    if (type == QStringLiteral("intrusion"))        return AppelMessages::EventKey::Intrusion;
    if (type == QStringLiteral("bousculade"))       return AppelMessages::EventKey::Bousculade;
    if (type == QStringLiteral("saturation"))       return AppelMessages::EventKey::Saturation;
    if (type == QStringLiteral("redirection"))      return AppelMessages::EventKey::Redirection;
    if (type == QStringLiteral("attente"))          return AppelMessages::EventKey::Attente;
    if (type == QStringLiteral("flux_sortie"))      return AppelMessages::EventKey::FluxSortie;
    if (type == QStringLiteral("immobile"))         return AppelMessages::EventKey::Immobile;
    return AppelMessages::EventKey::RetourNormal;
}

bool AppelManager::verifierAntiSpam(const QString& cle) const
{
    if (!m_antiSpam.contains(cle))
        return true;

    const AntiSpam& spam = m_antiSpam.value(cle);
    const qint64 elapsed = QDateTime::currentDateTime().toMSecsSinceEpoch() - spam.dernierAppel.toMSecsSinceEpoch();

    if (elapsed < ANTISPAM_DELAI_MS && spam.compteur >= ANTISPAM_MAX_PAR_FENETRE)
        return false;

    return true;
}

void AppelManager::marquerAntiSpam(const QString& cle)
{
    AntiSpam& spam = m_antiSpam[cle];
    spam.dernierAppel = QDateTime::currentDateTime();
    spam.compteur++;
}

void AppelManager::surAppelChange(const AriClient::AppelInfo& info)
{
    // Trouver la salle associée à cet appel (via channelId stocké quelque part)
    // Pour simplifier, on émet le signal générique
    emit statutAppelChange("", info.channelId, info.etat, info.extension);
}

void AppelManager::surAppelTermine(const QString& channelId, bool succes)
{
    emit log(QString("Appel terminé: %1 (%2)").arg(channelId, succes ? "succès" : "échec"));
}

void AppelManager::surMessageEnvoye(const QString& extension, bool succes, const QString& details)
{
    emit log(QString("Message SIP: %1 → %2 (%3)").arg(succes ? "OK" : "ÉCHEC", extension, details));
}

void AppelManager::surWavGenere(const QString& chemin)
{
    // Log silencieux, le nettoyage se fait dans TtsWav
}

void AppelManager::surErreurAri(const QString& msg)
{
    emit erreur(QString("ARI: %1").arg(msg));
}

void AppelManager::surErreurAmi(const QString& msg)
{
    emit erreur(QString("AMI: %1").arg(msg));
}

void AppelManager::surConnexionAriChangee(bool connecte)
{
    emit log(QString("ARI: %1").arg(connecte ? "connecté" : "déconnecté"));
}

void AppelManager::surConnexionAmiChangee(bool connecte)
{
    emit log(QString("AMI: %1").arg(connecte ? "connecté" : "déconnecté"));
}