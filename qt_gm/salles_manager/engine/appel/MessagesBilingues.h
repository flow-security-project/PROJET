#pragma once

#include <QString>

namespace AppelMessages {

// Clés d'événements pour la table bilingue
enum class EventKey {
    Evacuation,
    Intrusion,
    Bousculade,
    Saturation,
    Redirection,
    Attente,
    FluxSortie,
    Immobile,
    RetourNormal,
    TestVocale
};

// Texte FR/EN pour chaque événement
// Les %1, %2, %3 sont des placeholders pour QString::arg()
struct TexteBilingue {
    const char* fr;
    const char* en;
};

// Table centrale des messages (FR, EN)
inline const TexteBilingue TABLE[] = {
    // Evacuation
    { "Évacuation active dans la salle %1 ! Sortez immédiatement par la porte !",
      "Evacuation active in room %1! Leave immediately through the door!" },

    // Intrusion
    { "Alerte intrusion dans la salle %1. Présence hors horaires autorisés.",
      "Intrusion alert in room %1. Presence outside authorized hours." },

    // Bousculade
    { "Risque de bousculade dans la salle %1.",
      "Risk of stampede in room %1." },

    // Saturation
    { "La salle %1 est saturée !",
      "Room %1 is full!" },

    // Redirection (UNI-MARKET)
    { "La porte %1 est saturée. Redirection vers la porte %2.",
      "Door %1 is full. Redirecting to door %2." },

    // Attente (MULTI-MARKET)
    { "La salle %1 est saturée. Attente estimée %2 minutes.",
      "Room %1 is full. Estimated wait: %2 minutes." },

    // Flux de sortie anormal
    { "Flux de sortie anormal à la porte %1.",
      "Abnormal exit flow at door %1." },

    // Personne immobile
    { "Personne immobile détectée dans la salle %1.",
      "Motionless person detected in room %1." },

    // Retour à la normale
    { "La salle %1 est de nouveau accessible.",
      "Room %1 is accessible again." },

    // Test vocal
    { "Test vocal du système de surveillance.",
      "Voice test of the surveillance system." }
};

// Récupère le texte dans la langue demandée
inline QString texte(EventKey key, const QString& langue = "fr") {
    const TexteBilingue& t = TABLE[static_cast<int>(key)];
    return (langue == "en") ? QString::fromUtf8(t.en) : QString::fromUtf8(t.fr);
}

// Version avec arguments (FR/EN)
inline QString texte(EventKey key, const QString& langue, const QString& arg1) {
    return texte(key, langue).arg(arg1);
}
inline QString texte(EventKey key, const QString& langue, const QString& arg1, const QString& arg2) {
    return texte(key, langue).arg(arg1, arg2);
}
inline QString texte(EventKey key, const QString& langue, const QString& arg1, const QString& arg2, const QString& arg3) {
    return texte(key, langue).arg(arg1, arg2, arg3);
}

// Texte pour attente avec gestion "indéterminée"
inline QString texteAttente(const QString& langue, const QString& nomSalle, double attenteMin) {
    if (attenteMin >= 0.0) {
        return texte(EventKey::Attente, langue).arg(nomSalle, QString::number(int(attenteMin + 0.5)));
    } else {
        const char* fr = "La salle %1 est saturée. Attente indéterminée.";
        const char* en = "Room %1 is full. Estimated wait unknown.";
        return (langue == "en") ? QString::fromUtf8(en).arg(nomSalle)
                                : QString::fromUtf8(fr).arg(nomSalle);
    }
}

// Texte pour redirection
inline QString texteRedirection(const QString& langue, const QString& porteSrc, const QString& porteDst) {
    return texte(EventKey::Redirection, langue).arg(porteSrc, porteDst);
}

} // namespace AppelMessages