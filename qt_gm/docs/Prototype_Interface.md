Voici le prototype complet et détaillé de l'interface Qt Creator pour **i++ v4.0**, avec toutes les sous-fenêtres, boutons et types de sortie explicités. Ce document est prêt à servir de spécification d'implémentation.

---

## Prototype Interface Qt i++ v4.0 — Vue Globale

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  BARRE D'ÉTAT GLOBAL                                                        │
│  ÉVACUATION: [🟢 INACTIF] | MQTT: 🟢 Connecté | Asterisk: 🟢 Enregistré    │
├──────────────────────────────┬──────────────────────────────────────────────┤
│                              │                                              │
│  ZONE SUPERVISION            │  ZONE DÉTAIL SALLE SÉLECTIONNÉE              │
│  MULTI-SALLES                │  ┌──────────────────────────────────────────┐│
│  ┌──────────┐ ┌──────────┐   │  │ ONGLETS:                                 ││
│  │ Salle A  │ │ Salle B  │   │  │ [Visualisation] [Configuration] [Alertes]││
│  │ 🟢 12/30 │ │ 🟠 28/30 │   │  ├──────────────────────────────────────────┤│
│  │ LED:Vert │ │ LED:Orng │   │  │                                          ││
│  └──────────┘ └──────────┘   │  │  (Contenu onglet actif ci-dessous)       ││
│  ┌──────────┐ ┌──────────┐   │  │                                          ││
│  │ Salle C  │ │ Salle D  │   │  │                                          ││
│  │ 🔴 30/30 │ │ ⚫ H.LIGNE│   │  │                                          ││
│  │ LED:Rge  │ │          │   │  │                                          ││
│  └──────────┘ └──────────┘   │  └──────────────────────────────────────────┘│
│                              │                                              │
├──────────────────────────────┴──────────────────────────────────────────────┤
│  PANNEAU ALERTES UNIFIÉ                                                     │
│  🔴 14:23 ÉVACUATION AUTO B204 | Audio✅ Surface✅ | Appel Sécu ✅ Terminé  │
│  ⚠️ 14:20 Bousculade A102 score 0.94 | Audio✅ ToF✅ | Appel Sécu 🟡 En cours│
│  👤 14:18 Immobile Couloir Est 6min | Appel Infirmerie ✅ Terminé           │
│  [Filtrer ▼] [Exporter CSV] [Acquitter Sélection]                          │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Sous-Fenêtres Détaillées par Onglet/Bouton

### 1. Onglet VISUALISATION (Zone Détail Salle)

C'est l'onglet par défaut au clic sur une salle dans la grille.

```
┌──────────────────────────────────────────────────────────────────┐
│  COURBE TEMPS RÉEL (QCustomPlot)                                │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  ▲ Occupation                                             │ │
│  │  │  ╭─╮      ╭──╮                                        │ │
│  │  │ ╭╯ ╰╮   ╭╯  ╰╮  ← Courbe comptage A-B (ligne bleue)  │ │
│  │  │╭╯   ╰───╯    ╰╮                                       │ │
│  │  │▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ ← Aire densité estimée (orange transp)│ │
│  │  └──────────────────────────────────► Temps               │ │
│  │  Régime actuel: [CLUSTERING] | Confiance: 97%             │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ANTICIPATION SATURATION                                        │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ Saturation prévue dans: 8 min                              │ │
│  │ ████████████████░░░░░░░░ 78% → Tendance: +3.2 pers/min    │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  CONDITIONS ÉVACUATION (Auto)                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ 🔊 Audio foule     : ████░░ 87%  (seuil: P99)            │ │
│  │ 🌡️ Gradient T°     : ██░░░░ 42%  (seuil: 2°C/min)        │ │
│  │ 🚪 Saturation porte: █████░ 98% ✅ (seuil: 95%)           │ │
│  │                                                            │ │
│  │ Score fusion: 2/3 → ÉVACUATION ACTIVE depuis 2min 14s    │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  REFLET LCD LOCAL                                               │
│  ┌────────────────────┐                                         │
│  │ Ligne 1: Occ: 28/30│  ← Miroir exact LCD 16x2 boîtier     │ │
│  │ Ligne 2: EVACUATION│                                         │
│  └────────────────────┘                                         │
└──────────────────────────────────────────────────────────────────┘
```

**Types de sortie visuels :**
-   QCustomPlot : courbe ligne + aire remplie, scroll temps réel 1Hz
-   QProgressBar custom : barre anticipation avec gradient couleur
-   Barres conditions évacuation : progression vers seuil + checkmark vert quand atteint
-   QLabel reflet LCD : police monospace, bordure grise, mise à jour temps réel

---

### 2. Onglet CONFIGURATION (Zone Détail Salle)

```
┌──────────────────────────────────────────────────────────────────┐
│  CONFIGURATION SALLE: B204                                      │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ Nom de la salle:  [B204________________]                   │ │
│  │ Capacité max:     [30___] personnes                        │ │
│  │ Horaire début:    [07:00] ▼                                │ │
│  │ Horaire fin:      [22:00] ▼                                │ │
│  │ Seuil évacuation: [95___] %                                │ │
│  │                                                            │ │
│  │ [Envoyer Configuration]  [Réinitialiser]                   │ │
│  │                                                            │ │
│  │ Statut dernière commande: ✅ Confirmée 14:15:32            │ │
│  │ Topic: salle/B204/config/set | QoS: 1                      │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  TESTS MAINTENANCE                                              │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ [Tester LED Verte] [Tester LED Rouge] [Tester Stroboscope] │ │
│  │ [Tester LCD Ligne1] [Tester LCD Ligne2]                    │ │
│  │ [Forcer Évacuation] [Reset Alertes]                        │ │
│  │                                                            │ │
│  │ Résultat dernier test: LED Rouge OK (latence: 45ms)        │ │
│  └────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────┘
```

**Types de sortie/interaction :**
-   QFormLayout avec QValidator (capacité numérique 1-999, horaire QTimeEdit)
-   QPushButton envoi désactivé tant que formulaire invalide
-   QLabel statut commande avec horodatage + topic MQTT
-   Boutons test maintenance : envoi commande `salle/{id}/test` + feedback latence

---

### 3. Onglet ALERTES SPÉCIFIQUES SALLE (Zone Détail Salle)

```
┌──────────────────────────────────────────────────────────────────┐
│  HISTORIQUE ALERTES SALLE: B204                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ Heure  │ Type          │ Capteurs   │ Score │ Appel       │ │
│  │────────│───────────────│────────────│───────│─────────────│ │
│  │ 14:23  │ Évacuation    │ Audio+Surf │ 2/3   │ ✅ Sécu     │ │
│  │ 14:20  │ Bousculade    │ Audio+ToF  │ 0.94  │ 🟡 En cours │ │
│  │ 13:45  │ Saturation    │ ToF        │ 96%   │ ✅ Gestion  │ │
│  │ 11:30  │ Immobile      │ ToF        │ 5m12s │ ✅ Infirmer │ │
│  │ 09:15  │ Intrusion     │ A-B        │ 2m30s │ ❌ Échoué   │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  FILTRES: [Type ▼] [Date ▼] [Capteur ▼]                        │
│  [Exporter CSV cette salle] [Imprimer]                          │
└──────────────────────────────────────────────────────────────────┘
```

**Types de sortie :**
-   QTableWidget colonnes triables, lignes colorées par sévérité
-   Badges couleur : 🔴 rouge (évacuation), ⚠️ orange (bousculade/intrusion), 👤 jaune (immobile)
-   Statut appel : ✅ terminé vert, 🟡 en cours jaune, ❌ échoué rouge
-   Export CSV filtré par salle actuelle

---

### 4. Panneau Alertes Unifié (Zone Bas Globale)

```
┌──────────────────────────────────────────────────────────────────┐
│  ALERTES TEMPS RÉEL (Toutes Salles)                              │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ 🔴 14:23:07 │ B204 │ ÉVACUATION AUTO                      │ │
│  │              │ Déclencheurs: Audio(94%) + Surface(98%)     │ │
│  │              │ Appel Agent Sécu: ✅ Terminé 14:23:12       │ │
│  │              │ [Voir Détail] [Acquitter]                   │ │
│  ├────────────────────────────────────────────────────────────┤ │
│  │ ⚠️ 14:20:33 │ A102 │ BOUSCULADE                           │ │
│  │              │ Score: 0.94 | Capteurs: Audio + ToF         │ │
│  │              │ Appel Sécu: 🟡 En cours (00:45)             │ │
│  │              │ [Voir Détail] [Acquitter]                   │ │
│  ├────────────────────────────────────────────────────────────┤ │
│  │ 👤 14:18:12 │ C-Est│ PERSONNE IMMOBILE                    │ │
│  │              │ Durée: 6min 12s | Occupation: 1             │ │
│  │              │ Appel Infirmerie: ✅ Terminé 14:18:20       │ │
│  │              │ [Voir Détail] [Acquitter]                   │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  FILTRES GLOBAUX: [Salle ▼] [Type ▼] [Sévérité ▼] [Non acquittés uniquement ☑] │
│  [Exporter Tout CSV] [Acquitter Sélectionnés] [Purger Anciens >24h]             │
└──────────────────────────────────────────────────────────────────┘
```

**Types de sortie/interaction :**
-   QListView custom avec widgets imbriqués (pas simple QTableWidget pour richesse affichage)
-   Chaque alerte = widget expandable avec détails capteurs + statut appel
-   Bouton "Voir Détail" → sélectionne automatiquement la salle dans Zone 2 + ouvre onglet Alertes
-   Bouton "Acquitter" → marque traitée + log opérateur + changement couleur gris
-   Checkbox "Non acquittés uniquement" → filtre dynamique
-   Export CSV global avec toutes métadonnées

---

### 5. Barre d'État Global (Zone Haut)

```
┌──────────────────────────────────────────────────────────────────┐
│ ÉVACUATION: [🟢 INACTIF] │ MQTT: 🟢 Connecté │ Asterisk: 🟢 SIP │
│ Nœuds: 4/5 en ligne      │ Dernier msg: 14:23:07               │
└──────────────────────────────────────────────────────────────────┘
```

**États possibles :**
-   Évacuation : 🟢 INACTIF / 🔴 ACTIVE (clignotant) / 🟡 PARTIELLE (certaines salles seulement)
-   MQTT : 🟢 Connecté / 🟡 Reconnexion... / 🔴 Déconnecté
-   Asterisk : 🟢 Enregistré / 🟡 En appel / 🔴 Hors service
-   Nœuds : X/Y en ligne (clignote orange si Y-X > 0)

---

## Mapping Boutons ↔ Actions MQTT

| Bouton Interface | Action Utilisateur | Topic MQTT Publié | Payload | Feedback Visuel |
| :--- | :--- | :--- | :--- | :--- |
| Envoyer Configuration | Clic après saisie valide | `salle/{id}/config/set` | JSON config complet | Bouton désactivé 2s + label statut ✅/❌ |
| Tester LED/LCD | Clic bouton test | `salle/{id}/test` | `{"composant":"led","valeur":"rouge"}` | Label résultat + latence ms |
| Forcer Évacuation | Clic bouton maintenance | `salle/{id}/evacuation/force` | `{"active":true}` | Conditions évacuation passent à 3/3 override |
| Reset Alertes | Clic bouton maintenance | `salle/{id}/alerte/reset` | `{"type":"all"}` | Badge alertes salle effacé |
| Acquitter Alerte | Clic ligne alerte | `backend/alerte/acquit` (interne) | ID alerte + timestamp | Ligne grisée + log opérateur |
| Voir Détail Alerte | Clic ligne alerte | — (action UI locale) | — | Sélection salle Zone 2 + onglet Alertes |
| Exporter CSV | Clic bouton export | — (action locale backend) | — | QFileDialog save + confirmation |
| Filtrer Alertes | Sélection combo/check | — (filtrage UI local) | — | Liste mise à jour instantanément |

---

## Types de Sortie Qt Utilisés

| Type Sortie | Composant Qt | Usage dans i++ |
| :--- | :--- | :--- |
| Graphique temps réel | QCustomPlot | Courbe occupation + densité (Onglet Visualisation) |
| Barre progression custom | QProgressBar + QPainter | Anticipation saturation + conditions évacuation |
| Tableau données | QTableWidget | Historique alertes salle, configuration |
| Liste riche custom | QListView + QWidgetItemDelegate | Panneau alertes unifié avec détails expandables |
| Formulaire validé | QFormLayout + QValidator | Configuration salle |
| Indicateur état | QLabel + QPixmap icône | Barre état global, reflet LCD, statut commandes |
| Feedback temporel | QTimer + QLabel | Latence tests, durée appels, countdown anticipation |
| Dialog fichier | QFileDialog | Export CSV/PDF |
| Animation | QGraphicsEffect + QTimer | Clignotement évacuation, transition états |
| Notification sonore PC | QSoundEffect / Asterisk TTS | Alarme locale dashboard (optionnel, complémentaire Asterisk) |

Ce prototype couvre **100% des 19 fonctionnalités** avec une spécification d'implémentation précise. Chaque élément d'interface a un rôle fonctionnel défini, un composant Qt identifié, et un lien explicite avec les signaux MQTT. Souhaitez-vous que je génère le code C++ squelette de la classe principale `MainWindow` correspondant à ce prototype ?
