#pragma once

#include <QObject>
#include <QString>

// Détecteur de passage directionnel "A-B" partagé entre la source MQTT et la
// source Démo :
//   - Capteur A (VL53L0X / ToF) : à l'extérieur, devant la porte.
//   - Capteur B (HC-SR04 / ultrason) : à l'intérieur, derrière la porte.
// Validation : A puis B (entrée) ; B puis A (sortie). Toute séquence
// incomplète est annulée après un délai (fenêtre) sans le second déclencheur.
class PassageDetectorAB : public QObject
{
    Q_OBJECT

public:
    enum class Etat { Attente, VuA, VuB };

    explicit PassageDetectorAB(QObject* parent = nullptr);

    // Front montant du capteur A : ToF bloqué (détection faite par l'appelant).
    void majToF(bool bloque, qint64 tMs);
    // Front montant du capteur B : événement "presence" de l'ultrason.
    void declencherUltrason(qint64 tMs);
    // Annule une séquence en attente dont la fenêtre est dépassée.
    void verifierExpiration(qint64 maintenantMs);
    // Réinitialise la séquence en cours (pas l'état du ToF).
    void reset();

    Etat etat() const { return m_etat; }

    void setFenetreMs(qint64 ms) { m_fenetreMs = ms; }
    void setDebounce(int echantillons) { m_debounce = echantillons; }

signals:
    void passageValide(const QString& direction); // "entree" | "sortie"
    void capteurAActive();
    void capteurBActive();
    void sequenceAnnulee();

private:
    Etat m_etat = Etat::Attente;
    qint64 m_dernierDeclenchementMs = 0;
    bool m_tofBloque = false;   // front montant ToF déjà compté
    int m_tofBloqueCompte = 0;  // échantillons ToF bloqués consécutifs
    qint64 m_fenetreMs = 5000;
    int m_debounce = 2;
};
