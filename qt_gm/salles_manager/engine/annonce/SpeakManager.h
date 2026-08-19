#pragma once

#include <QHash>
#include <QObject>
#include <QQueue>
#include <QTimer>

#include "engine/annonce/AnnonceVocale.h"

class DataSource;
struct Alerte;
struct Salle;

struct Annonce
{
    int priorite = 0;   // 1 = critique (évacuation) ... 7 = info (retour à la normale)
    QString cle;        // clé anti-spam (salle + type d'événement)
    QString texte;
    QString langue = QStringLiteral("fr");
};

// Annonceur vocal automatique du superviseur.
// Surveille l'état de chaque salle (front montant uniquement) et les alertes
// de la source, puis lit les messages (français ou anglais selon la
// configuration de la salle) avec une file de priorité : une seule voix,
// les annonces critiques interrompent les autres.
class SpeakManager : public QObject
{
    Q_OBJECT

public:
    explicit SpeakManager(QObject* parent = nullptr);

    void setSource(DataSource* source);

    bool active() const { return m_actif; }
    void setActif(bool actif);

    QString langueGlobale() const { return m_langueGlobale; }
    void setLangueGlobale(const QString& langue);

    QString backend() const { return m_voix.backend(); }
    bool voixDisponible() const { return m_voix.disponible(); }

    // Annonce manuelle (bouton TEST) : aucune contrainte de cooldown.
    void annoncer(const QString& texte);
    // Annonce de test de la synthèse vocale, dans la langue globale.
    void testVoix();

signals:
    void annonceEnoncee(const QString& texte);
    void annonceFilee(const QString& texte);

private slots:
    void onTick();

private:
    void surveillerSalles();
    void onAlerte(const Alerte& alerte);
    void ajouter(int priorite, const QString& cle, const QString& texte,
                 const QString& langue, bool repetitif = false);
    void insererDansFile(const Annonce& annonce);
    void jouerSuivante();
    void onVoixTerminee();
    static QString nomSalle(const Salle& salle);
    static QString langueSalle(const Salle& salle, const QString& defaut);

    static QString texte(const QString& langue,
                         const QString& fr, const QString& en);

    DataSource* m_source = nullptr;
    AnnonceVocale m_voix;
    QTimer m_timer;
    bool m_actif = true;
    QString m_langueGlobale = QStringLiteral("fr");
    QQueue<Annonce> m_file;
    Annonce m_enCours;
    QHash<QString, qint64> m_dernierParTypeMs;      // cooldown 60 s (front montant)
    QHash<QString, qint64> m_dernierRepetitionMs;   // répétition 60 s (état persistant)
    QHash<QString, QString> m_decisionAvant;
    QHash<QString, bool> m_evacAvant;
    QHash<QString, bool> m_satureAvant;
};