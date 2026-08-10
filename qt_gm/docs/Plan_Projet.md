# i++ v4.0 — Plan de Projet : Architecture du Code & Phases de Développement

> **Principe d'architecture retenu :** l'ESP32 est un simple relais de données (capture pure), à l'exception d'un pré-traitement audio minimal. **100 % du traitement, de l'intelligence décisionnelle et de l'IHM vivent dans une application Qt 6 monolithique (C++)**, qui intègre également le client ARI pour les appels Asterisk.

---

## Table des matières

- [Vue d'ensemble](#vue-densemble)
- [Architecture du code](#architecture-du-code)
  - [Schéma global](#schéma-global)
  - [ESP32 — capture pure](#esp32--capture-pure)
  - [Application Qt monolithique](#application-qt-monolithique)
  - [Topics MQTT](#topics-mqtt)
  - [Chaîne de traitement Qt](#chaîne-de-traitement-qt)
  - [Dépendances entre sous-systèmes](#dépendances-entre-sous-systèmes)
- [Phases de Développement](#phases-de-développement)
- [Annexe — Divergences avec les documents existants](#annexe--divergences-avec-les-documents-existants)

---

## Vue d'ensemble

| Sous-système | Rôle | Intelligence |
| :--- | :--- | :--- |
| **ESP32 (nœud)** | Capture les données brutes des 5 capteurs + actionneurs (LED/LCD) | Aucune, sauf audio réduit |
| **Broker MQTT** | Acheminement TLS des données brutes et des commandes | Aucune |
| **App Qt 6 (PC)** | Tout le processing, l'IHM, les alertes, l'historique, les exports | 100 % |
| **Asterisk 20** | Passerelle voix/SIP, TTS, appels automatiques | Guidage vocal uniquement |

**Conséquences du choix monolithe Qt :**
- ESP32 minimal : flash quasi vide, firmware figé une fois la capture validée.
- Algorithmes modifiables sans reflasher les nœuds (mise à jour d'un seul exécutable PC).
- Un seul langage (C++) pour traiter, décider et afficher.
- **Seul compromis :** l'audio INMP441 subit un pré-traitement minimal sur ESP32 (RMS/FFT réduit à ~1 Hz), sinon le flux I2S brut saturerait le réseau (~256 kbit/s par nœud).

---

## Architecture du code

### Schéma global

```
┌────────────────────────────── POSTE CENTRAL (PC) ──────────────────────────────┐
│                                                                                │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │  APPLICATION Qt 6 (C++) — MONOLITHE : Traitement + IHM + ARI            │  │
│  │                                                                          │  │
│  │  [1] PROCESSING ENGINE (toute l'intelligence)                            │  │
│  │   ├─ ingestor/      → consommation MQTT brut, validation, buffers        │  │
│  │   ├─ filtrage/      → médian, lissage, fenêtres                          │  │
│  │   ├─ ab_system/     → séquence A-B, anti-rebond, comptage E/S (F16)      │  │
│  │   ├─ occupation/    → occupation, capacité, anticipation (F2)            │  │
│  │   ├─ densite/       → clustering + surface + lookup table calibrée (F17) │  │
│  │   ├─ securite/      → intrusion horaires (F11)                           │  │
│  │   ├─ alerts/        → file d'alertes, acquittement, historique, export   │  │
│  │   └─ ari_client/    → appels Asterisk TTS (F13) + statut appels          │  │
│  │                                                                          │  │
│  │  [2] UI LAYER (charte Corporate, prototype approuvé)                     │  │
│  │   ├─ status_bar/    → MQTT, Asterisk, nœuds en ligne                     │  │
│  │   ├─ salle_grid/    → supervision multi-salles, hors ligne "NON FIABLE"  │  │
│  │   ├─ visualisation/ → QCustomPlot A-B + densité + régime (F5,F19)        │  │
│  │   ├─ anticipation/  → barre "saturation prévue dans X min" (F2)          │  │
│  │   ├─ lcd_mirror/    → reflet LCD 16x2 temps réel (F8)                    │  │
│  │   ├─ configuration/ → formulaire + tests maintenance (F7)                │  │
│  │   ├─ alert_panel/   → alertes unifiées, filtres, acquittement (F3,F13)   │  │
│  │   └─ resources/     → charte graphique .qss Corporate Engineering        │  │
│  └──────────────────────────────┬───────────────────────────────────────────┘  │
└─────────────────────────────────┼──────────────────────────────────────────────┘
                                  │ ARI REST + WS (Stasis)
                 ┌────────────────┴───────────────┐
                 ▼                                ▼
        ┌────────────────────┐          ┌──────────────────────┐
        │ BROKER MQTT TLS    │          │ ASTERISK 20 + ARI    │
        │ (Mosquitto/EMQX)   │          │ Stasis, TTS, SIP     │
        └──────────┬─────────┘          └──────────────────────┘
                   │ MQTT over TLS — topics données BRUTES
        ┌──────────┴───────────┐      ┌───────────────────┐
        │ NŒUD ESP32 #1        │  ... │ NŒUD ESP32 #N      │
        │ CAPTURE UNIQUEMENT   │      └───────────────────┘
        └──────────────────────┘
```

### ESP32 — capture pure

```
main.ino ── setup() / loop()
   ├── drivers/              → lecture brute uniquement
   │    ├── vl53l0x_drv      → distance mm + target_status    (I2C, 5 Hz)
   │    ├── hcsr04_drv       → distance cm + timestamp        (GPIO)
   │    ├── inmp441_drv      → échantillons I2S bruts         (I2S)
   │    ├── sht4x_drv        → température + humidité         (I2C, 0.5 Hz)
   │    ├── led_rgb          → commande réception (aucune logique)
   │    └── lcd16x2          → affichage texte reçu
   │
   ├── audio_light/          → UNIQUE pré-traitement autorisé (sinon réseau saturé)
   │    ├── rms_glissant     → RMS sur fenêtre 1 s
   │    ├── fft_bandes       → FFT 200-4000 Hz, bande cris 800-3000 Hz
   │    └── percentile99     → percentile dynamique de l'historique RMS
   │
   ├── capture/              → scheduler cadencé par capteur
   │    ├── scheduler        → échantillonnage périodique 5 Hz / 0.5 Hz / 1 Hz
   │    └── timestamp        → horodatage ms des trames
   │
   ├── net/
   │    ├── mqtt_publisher   → publie les trames brutes + audio réduit (TLS, QoS, reconnect)
   │    └── command_handler  → reçoit config / test / force évacuation / reset
   │
   └── config/               → stocke uniquement id salle, capacité max,
                                horaires (RAM, aucun calcul)
```

### Application Qt monolithique

```
dashboard/
 ├── main.cpp
 ├── engine/                       ← [1] PROCESSING ENGINE (sans UI, testable seul)
 │    ├── ingestor/                → QMqttClient, validation payloads, buffers par salle
 │    ├── filtrage/                → médian 3 trames, lissage, fenêtres glissantes
│    ├── ab_system/               → détection séquence A-B, anti-rebond,
│    │                               comptage entrée/sortie (F16)
│    ├── occupation/              → occupation courante, anticipation tendance (F2)
│    ├── densite/                 → clustering (régime bas 1-2 pers.),
│    │                               surface occupée + lookup table (régime haut) (F17)
│    ├── securite/                → intrusion hors horaires (F11)
│    ├── alerts/                  → file d'alertes, sévérité, acquittement,
│    │                               historique horodaté, export CSV/PDF (F4)
│    └── ari_client/              → Stasis app, création channel, playback TTS,
│                                    statut d'appel (F13)
 ├── ui/                           ← [2] UI LAYER (charte Corporate)
 │    ├── main_window              → splitter 3 zones : barre état / grille+détail / alertes
 │    ├── status_bar / salle_grid / visualisation (QCustomPlot) / anticipation /
 │    │   conditions_evac / lcd_mirror / configuration / alert_panel
 │    └── resources/               → charte graphique .qss (Corporate Engineering)
 ├── models/
 │    ├── salle_model              → QAbstractListModel des salles (état, occupation)
 │    └── alerte_model             → modèle alertes + filtres + acquittement
 ├── exports/                      → CSV/PDF (QFileDialog + génération)
 └── CMakeLists.txt
```

### Topics MQTT

#### Données brutes (ESP32 → Qt)

| Topic | Payload | Fréquence |
| :--- | :--- | :--- |
| `salle/{id}/raw/tof` | `{"d_mm":820,"status":0,"t_ms":1705312987000}` | 5 Hz |
| `salle/{id}/raw/ultrason` | `{"d_cm":34,"t_ms":1705312987012}` | événementiel |
| `salle/{id}/audio` | `{"rms":0.72,"band_cris":true,"p99":0.68,"t_ms":...}` | 1 Hz (pré-traité ESP32) |
| `salle/{id}/raw/env` | `{"t_c":28.4,"hr":62,"t_ms":1705312987000}` | 0.5 Hz |

> Seul le signal `audio` subit un pré-traitement minimal sur ESP32 (RMS/FFT/percentile99). Les autres signaux sont publiés **bruts** : tout le filtrage et l'interprétation se font dans Qt.

#### Commandes (Qt → ESP32)

| Commande | Topic | Payload | Action ESP32 |
| :--- | :--- | :--- | :--- |
| Configuration salle | `salle/{id}/config/set` | `{"nom":"B204","capacite":30,"horaires":{"debut":"07:00","fin":"22:00"}}` | Mise à jour RAM + confirmation |
| Forcer évacuation | `salle/{id}/evacuation/force` | `{"active":true}` | Stroboscope LED immédiat |
| Reset alerte | `salle/{id}/alerte/reset` | `{"type":"all"}` | Acquittement local |
| Test maintenance | `salle/{id}/test` | `{"composant":"led","valeur":"rouge"}` | Diagnostic LED/LCD |
| Demande état | `salle/{id}/etat/get` | `{}` | Réponse état capteurs |

#### Feedback actionneurs (ESP32 → Qt)

| Topic | Payload | Fréquence |
| :--- | :--- | :--- |
| `salle/{id}/led/etat` | `{"couleur":"orange","mode":"progressif","luminosite":80}` | sur changement |
| `salle/{id}/lcd/etat` | `{"ligne1":"Occ: 24/30","ligne2":"En ligne"}` | sur changement |

> L'app Qt affiche toujours l'état **confirmé** par le nœud, jamais l'état commandé.

### Chaîne de traitement Qt

```
MQTT brut ─► ingestor ─► filtrage ─► ab_system ─► occupation / anticipation
                                      │
                                      ├──► densite (clustering / surface calibrée)
                                      └──► securite (intrusion hors horaires) ─► alertes ─► ari_client
```

1. **Ingestor** : subscription wildcard `salle/+/raw/#`, validation JSON, buffers par salle.
2. **Filtrage** : nettoyage du signal avant toute interprétation.
3. **Système A-B** : corrélation temporelle VL53L0X ↔ HC-SR04 → direction validée (F16).
4. **Occupation** : cumul entrées − sorties, tendance (F2).
5. **Densité** : bascule automatique clustering ↔ surface selon régime (F17, F19).
6. **Sécurité** : intrusion hors horaires (F11).
7. **Alertes** : file horodatée → panneau Qt + commandes LED/LCD + appels Asterisk (F13).

### Dépendances entre sous-systèmes

```
ESP32 (capture + audio réduit)
   │  données BRUTES via MQTT TLS
   ▼
BROKER MQTT TLS
   │
   ▼
APPL Qt  ──► processing ──► alertes ──► exports CSV/PDF
   │          │               │
   │          │               └──► ari_client ──► ASTERISK (appels TTS, F13)
   │          ▼
   └──► commandes MQTT (config, LED, LCD) ──► ESP32 (feedback confirmé)
```

---

## Phases de Développement

### Phase 0 — Fondations & Environnement

| # | Tâche | Validation |
| :--- | :--- | :--- |
| 0.1 | Montage matériel : 5 capteurs + LED RGB + LCD 16x2 sur ESP32 | Schéma câblage vérifié |
| 0.2 | Environnement : PlatformIO, Qt 6 Creator + QCustomPlot, Mosquitto/EMQX | Builds OK |
| 0.3 | MQTT over TLS : CA + certificats device + broker | Test pub/sub chiffré |
| 0.4 | Asterisk 20 installé + module ARI activé | Ping ARI OK |

### Phase 1 — ESP32 Capture Pure

| # | Tâche | Validation |
| :--- | :--- | :--- |
| 1.1 | Drivers bruts VL53L0X / HC-SR04 / INMP441 / SHT4x | Logs série bruts |
| 1.2 | Scheduler de capture + horodatage ms | Trames cadencées |
| 1.3 | Pré-traitement audio minimal (RMS, FFT bandes, percentile99) | Données cohérentes |
| 1.4 | Publication MQTT brute (4 topics) | `mosquitto_sub` vérifie |
| 1.5 | Réception commandes (config, test LED/LCD, force évac, reset) | Round-trip OK |
| 1.6 | Feedback LED/LCD publié après application | État confirmé reçu |

### Phase 2 — Pipeline de Traitement Qt (F16, F2)

| # | Tâche | Validation |
| :--- | :--- | :--- |
| 2.1 | Ingestor MQTT : subscription `salle/+/raw/#`, validation, buffers | Trames loggées |
| 2.2 | Filtrage (médian, lissage, fenêtres) | Signal stabilisé |
| 2.3 | Système A-B directionnel + anti-rebond + comptage E/S | ≥97 % précision direction |
| 2.4 | Occupation + anticipation saturation (tendance) | Scénarios de flux |

### Phase 3 — Intelligence de Flux dans Qt (F17-F19)

| # | Tâche | Validation |
| :--- | :--- | :--- |
| 3.1 | Clustering (régime bas 1-2 pers.) + bascule régime | Tests 1-5 personnes |
| 3.2 | Estimation par surface occupée + lookup table chargée depuis fichier | Calibration initiale |
| 3.3 | Analyse audio Qt (RMS glissant, FFT, percentile99, bande cris) | Données labo |
| 3.4 | Gradient thermique dT/dt | Test source de chaleur |
| 3.5 | Indicateur de régime + score de confiance (F19) | Affichage vérifié |

### Phase 4 — Sécurité dans Qt (F3, F11)

| # | Tâche | Validation |
| :--- | :--- | :--- |
| 4.1 | Intrusion hors horaires (>2 min) | Scénario temporisé |
| 4.2 | Détection flux de sortie anormal (μ+3σ) (F3) | Scénario évacuation |
| 4.3 | Commandes maintenance via MQTT (test LED/LCD, force évac manuelle, reset alertes) | Round-trip OK |

### Phase 5 — Interface Dashboard Qt (F1-F8)

| # | Tâche | Validation |
| :--- | :--- | :--- |
| 5.1 | MainWindow 3 zones + barre d'état globale + charte QSS | Maquette conforme |
| 5.2 | Grille supervision multi-salles + "HORS LIGNE / NON FIABLE" (F6) | Données réelles |
| 5.3 | Onglet Visualisation : QCustomPlot A-B + densité + régime + anticipation | Temps réel 1 Hz |
| 5.4 | Onglet Configuration : formulaire validé + tests maintenance (F7) | ESP32 réagit |
| 5.5 | Panneau alertes unifié : filtres, acquittement, exports (F3, F13) | Scénarios d'alerte |
| 5.6 | Reflet LCD + LED miroir (F8) | État confirmé cohérent |
| 5.7 | Historique + export CSV/PDF (F4) | Fichiers vérifiés |

### Phase 6 — Intégration Asterisk (F13-F15)

| # | Tâche | Validation |
| :--- | :--- | :--- |
| 6.1 | Client ARI dans Qt : connexion Stasis, création channel | Ping ARI OK |
| 6.2 | Génération TTS + playback (2 scénarios : saturation critique, intrusion hors horaires) | Appel réel reçu |
| 6.3 | Statut d'appel (terminé / en cours / échoué) dans le panneau Qt | Panel à jour |
| 6.4 | Double canal humain/machine : message vocal + JSON conservé (F15) | Corrélation alerte ↔ appel |

### Phase 7 — Calibration, Tests & Livraison

| # | Tâche | Validation |
| :--- | :--- | :--- |
| 7.1 | Calibration in situ (F18) : ground truth 30 min + régression + lookup table Qt | Courbe calibrée |
| 7.2 | Tests bout-en-bout : flux réel, intrusion, saturation | Scénario complet |
| 7.3 | Tests non-fonctionnels : latence, reconnexion, panne nœud, débit MQTT | Critères OK |
| 7.4 | Documentation, rapport, démo jury | Présentation prête |

---

## Annexe — Divergences avec les documents existants

Le présent document fait foi pour l'implémentation. Il introduit trois écarts majeurs avec `Donnee_Capteur.md` et `Flux.md` :

| Document existant | Approche décrite | Approche retenue ici |
| :--- | :--- | :--- |
| `Donnee_Capteur.md` | Traitement embarqué sur ESP32 (filtrage, clustering, fusion, signaux synthétiques) | Capture brute sur ESP32, **tout le processing dans Qt** (sauf audio réduit) |
| `Donnee_Capteur.md` | Topics `salle/{id}/flux`, `salle/{id}/passage`, `salle/{id}/etat` (signaux sémantiques) | Topics `salle/{id}/raw/*` (données brutes) + traitement Qt |
| `Flux.md` | Backend séparé (Spring Boot / FastAPI) + intégration ARI backend | **Monolithe Qt C++** : processing + IHM + client ARI dans un seul programme |

**Les fichiers à mettre à jour pour cohérence :**
- `Donnee_Capteur.md` → réécrire en spec "données brutes → traitement Qt".
- `Flux.md` → section Stack : retirer le backend séparé, décrire le monolithe Qt.
- `Prototype_Interface.md` → inchangé (cible UI Qt identique).
- `Gestion Multi Personnes.md` → inchangé (algorithme identique, exécuté côté Qt).
