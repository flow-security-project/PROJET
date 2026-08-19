# Simulation.md — Documentation technique du gestionnaire de salles

Projet : `qt_gm/salles_manager` — superviseur de flux de personnes (portes/amphis)
alimenté en démo locale (`DemoSource`) ou en réel via MQTT (`MqttSource`).

---

## 1. Vue d'ensemble

Le logiciel supervise des **salles** (portes d'amphi, portails de stade...) :
occupation, densité, flux entrées/sorties, anticipation de saturation, intrusion,
flux de sortie anormal (F3), alertes, affichage LED progressive (F1) et LCD 16x2 (F8),
et historique persistant.

Deux sources de données interchangeables, derrière l'interface `DataSource` :

| Source | Rôle |
|---|---|
| `data/DemoSource` | Simulation locale complète : capteur ToF simulé à 5 Hz, passages A-B, scénarios de flux, alertes F3/F11, LED/LCD. Aucun matériel requis. |
| `data/MqttSource` | Connexion réelle à des ESP32 par MQTT : abonnement aux trames capteurs, mesure de hauteur de porte, décisions de flux republiées, watchdog. |

Moteurs de traitement (dossier `engine/`) :

| Moteur | Rôle |
|---|---|
| `engine/densite/DensiteEstimator` | Estimation de la densité depuis les trames ToF (2 régimes : clustering / surface), calibration F18 |
| `engine/passage/PassageDetectorAB` | Machine à états A-B : A = ToF (extérieur), B = ultrason (intérieur) ; A→B = entrée, B→A = sortie |
| `engine/securite/IntrusionDetector` | Détection de présence hors horaires autorisés (alerte après 120 s) |
| `engine/flux/FluxOrchestrator` | Décisions de flux : normal / attente (MULTI) / redirection (UNI) |

---

## 2. Arborescence du projet

```
salles_manager/
├── main.cpp                    # point d'entrée + harnais SM_SHOT
├── CMakeLists.txt              # cible salles_manager + test history_smoke
├── README.md
├── Simulation.md               # ce document
├── models/
│   ├── Salle.h                 # structure Salle + calculs (taux, anticipation, LED/LCD, détection F3)
│   ├── Alerte.h                # structure Alerte + libellés/sévérités
│   ├── AlerteModel.h           # modèle d'alertes (QObject, pas un QAbstractItemModel)
│   └── Groupe.h                # ModeFlux (Multi/Uni) + Groupe
├── history/
│   └── HistoryManager.{h,cpp}  # historique CSV par minute, passages, alertes
├── engine/
│   ├── densite/DensiteEstimator.{h,cpp}
│   ├── securite/IntrusionDetector.{h,cpp}
│   ├── flux/FluxOrchestrator.{h,cpp}
│   └── passage/PassageDetectorAB.{h,cpp}
├── data/
│   ├── DataSource.{h,cpp}      # classe de base abstraite + gestion des groupes
│   ├── DemoSource.{h,cpp}      # simulation
│   ├── MqttSource.{h,cpp}      # source réelle
│   └── MqttClient.{h,cpp}      # client MQTT 3.1.1 maison sur QTcpSocket
├── ui/                         # widgets (MainWindow, SallesWidget, SalleGrid, SalleConfigWidget,
│                               #  StadeWidget, SalleDetailWidget, AbSystemWidget, LedLcdWidget,
│                               #  IntegratedPlotWidget (QCustomPlot), JaugeSaturation, AlertePanelWidget)
├── resources/
│   ├── charte.qss              # thème graphique
│   └── resources.qrc           # :/styles/charte.qss
├── tests/
│   └── HistoryManagerSmoke.cpp # test du stockage historique
└── build/                      # compilation autonome
```

Dépendance externe : `../vendor/qcustomplot.{h,cpp}` (QCustomPlot) pour les courbes.

---

## 3. Fonctionnalités existantes

### Gestion des salles
- **Création** d'une salle (id unique obligatoire) : nom, capacité, seuil d'évacuation,
  horaires d'ouverture (début/fin), hauteur de porte, mode de flux, groupe.
- **Modification** : conserve occupation, densité, tendance, compteurs et historiques ;
  re-synchronise les horaires d'intrusion et la hauteur pour l'estimateur.
- **Suppression** (avec confirmation) : nettoie tous les moteurs et accumulateurs.
- **Actualisation** (demande d'état), **masquage / restauration** des cartes.
- **Mesure de la hauteur de porte** : en démo → valeur 205 à 225 cm après 650 ms ;
  en MQTT → médiane de 7 trames ToF valides collectées sur 3,5 s.
- **Groupes / stades** : `Groupe {id, nom, mode: Uni|Multi, seuilEcart}` ; suppression
  d'un groupe = suppression de ses salles membres. Mode UNI = portails interchangeables
  (redirection), MULTI = salles indépendantes (attente estimée).

### Supervision temps réel
- Occupation `occ/cap`, taux de remplissage, densité estimée (régime + confiance),
  tendance (pers/min), **anticipation de saturation en minutes**, débit instantané.
- Compteurs d'entrées/sorties, flux de sortie horodaté (histogramme glissant 600 s).
- Décisions de flux : `normal` | `attente` | `redirection` (voir §7).
- LED progressive (F1) : vert < 60 %, jaune ≥ 60 %, orange ≥ 80 %, rouge ≥ 95 %,
  éteint si hors ligne/attente, rouge + stroboscope si évacuation ou intrusion.
- LCD 16x2 (F8) : `<id> occ/cap` + `OK 07:00-22:00` ; variantes ATTENTE / HORS LIGNE /
  EVACUATION / INTRUSION.
- Système A-B visualisé (vue de dessus de la porte) : état `attente`/`vu_a`/`vu_b` +
  animation du dernier passage validé.

### Alertes
- **F3 — flux de sortie anormal** : débit sortant > μ + 3σ, min 5 pers/min, fenêtre 600 s,
  minimum 60 points. Latched : une seule alerte par épisode.
- **F11 — intrusion** : présence hors horaires autorisés pendant ≥ 120 s (capteurs
  HC-SR04 + VL53L0X, appel cible « Agent surveillance »).
- Modèle `AlerteModel` : ajout, modification, acquittement, purge, liste par salle.
- Persistance dans `alertes_history.csv` (rechargée au démarrage).
- Sévérités : `evacuation` = critique, `immobile` = info, autres = attention.

### Historique (HistoryManager)
- 3 fichiers par salle dans `QStandardPaths::AppDataLocation/history` :
  - `salle_<id>_history.csv` : un point agrégé **par minute** (moyennes occupation,
    densité, flux sortie, tendance, confiance, compteurs, régime, nb d'observations).
  - `salle_<id>_passages.csv` : passages validés individuels (direction, occupation,
    compteurs au moment du passage) — écrits par lots de 64.
  - `alertes_history.csv` : alertes avec métadonnées d'appel et acquittement.
- Périodes Jour / Semaine / Mois (calcul des bornes : début de jour, lundi de la
  semaine, 1er du mois). Cache mémoire invalidé par empreinte (taille + date).
- Statistiques : moyenne occ/densité, pic et creux, nb d'entrées/sorties.
- Exports : CSV global d'une salle (3 types de lignes : `echantillon`, `passage`,
  `alerte`), CSV du panneau d'alertes, export PDF du détail.

### Interface
- Grille responsive de **cartes cliquables** (`salleCard`), vue stade UNI (`stadeCard`,
  dialogue `StadeWidget`), barre de statut, panneau d'alertes, bouton démo rapide.
- Détail d'une salle (dialogue) : occupation, taux, débit, compteurs, jauge de
  saturation, courbes QCustomPlot, historique Jour/Semaine/Mois, exports CSV/PDF,
  états A-B et LED/LCD, liste des alertes de la salle.
- Sélecteur de source (Démo / MQTT) + champ broker (IP, port) dans `MainWindow`.
- Plein écran (F11), thème QSS `charte.qss` + palette (fond `#EEF2F7`, accent `#2563EB`).

---

## 4. Mécanisme de simulation (DemoSource)

### 4.1 Principe général
Un `QTimer` d'intervalle **1000 ms** déclenche `onTick()`. Chaque seconde, pour
chaque salle en ligne, le simulateur :
1. choisit un **scénario de flux** déterministe par salle (hash de l'id) ;
2. convertit le flux (pers/min) en entrées/sorties par **accumulateur fractionnaire** ;
3. injecte les passages dans la **machine A-B** (simule A puis B pour entrée, B puis A pour sortie) ;
4. met à jour occupation, compteurs, tendance, densité ;
5. applique la **détection de flux de sortie anormal (F3)** sur le débit sortant ;
6. vérifie l'**intrusion (F11)** selon les horaires ;
7. simule le **ToF à 5 Hz** pour l'estimateur de densité ;
8. calcule l'**anticipation** (régression linéaire) et pousse l'historique RAM ;
9. met à jour **LED/LCD** et émet `salleMiseAJour` ;
10. en fin de boucle, applique l'**orchestrateur de flux** (`majDecisionsFlux`).

### 4.2 Scénarios de flux (déterministes par id : `qHash(id) % 6`)

| N° | Profil | Flux cible (pers/min) | Effet observé |
|---|---|---|---|
| 0 | Montée lente | `+1.2` | Remplissage progressif |
| 1 | Montée rapide | `+6.0` | Saturation rapide → anticipation |
| 2 | Va-et-vient | `(tick/60)%2==0 ? +2.0 : -1.0` | Alternance toutes les minutes |
| 3 | Descente | `-1.5` | Vidage |
| 4 | Remplissage puis sortie brusque | `+8.0` si tick < 75 ; **`-240.0`** si tick < 87 ; sinon `+0.3` | Déclenche l'alerte **F3** (~75 s de montée, puis 12 s d'évacuation à 240 pers/min) |
| 5 | Présence permanente | `+0.4`, occupation forcée ≥ 1 | Hors horaires → alerte **F11** |

Si `occupation >= capacite`, le flux est plafonné à `min(flux, -1.0)` (la salle
commence à se vider).

### 4.3 Accumulateur de flux
```cpp
m_fluxAccum[id] += flux / 60.0;      // flux en pers/min → pers par tick de 1 s
int pas = int(accum);                // partie entière = nb de passages ce tick
accum -= pas;                        // partie fractionnaire conservée
// pas > 0 : simulerEntree() x pas ; pas < 0 : simulerSortie() x |pas|
```

### 4.4 Injection dans la machine A-B
- `simulerEntree` : `majToF(false)` (dégagement) → `majToF(true)` ×2 (debounce) →
  `declencherUltrason()` → la machine émet `passageValide("entree")` (A→B).
- `simulerSortie` : `majToF(false)` → `declencherUltrason()` (B d'abord) →
  `majToF(true)` ×2 → `passageValide("sortie")` (B→A).
- À chaque validation : occupation ± 1 (bornée par capacité/0), compteurs,
  émissions `etatAB("attente")`, `passageValide(salleId, direction, ts)`,
  `salleMiseAJour` + logs « PASSAGE A-B ».

### 4.5 Détection de flux de sortie (F3)
Chaque tick : `sortieFlux = max(0, -flux)` (débit sortant du scénario, pers/min)
est injecté dans `Salle::majDetectionFluxSortie` :
- historique glissant 600 points (`fluxSortieHist`), mu/sigma recalculés à chaque point ;
- alerte si `debit >= 5.0 pers/min` **et** `debit > mu + 3·sigma`, avec **latched**
  (`fluxSortieAnormal`, une seule alerte par épisode, horodatée).
- Émission `Alerte{type="flux_sortie", capteurs={"HC-SR04","VL53L0X"}}` + log
  « ALERTE F3 ».

### 4.6 Intrusion (F11)
`verifierIntrusion(id, now)` : `presence = occupation > 0` ; passe par
`IntrusionDetector::verifier` (voir §6.3). À la première alerte : `Alerte{type="intrusion",
capteurs={"HC-SR04","VL53L0X"}, appelCible="Agent surveillance"}` + log « ALERTE F11 ».

### 4.7 ToF simulé (5 Hz) pour la densité
```cpp
presenceProb = clamp(occupation / 3.0, 0.0, 1.0);   // proba d'un passage actif
// persistance : un passage « actif » dure jusqu'à ce que alea > 0.80 (~1-2 s)
plongeon = 900.0 + min(occupation, 10) * 60.0;      // profondeur du plongeon (mm)
// 5 échantillons espacés de 200 ms (5 Hz) :
//   actif :   distanceMm = hauteurMm - plongeon - U(0,200)
//   inactif : distanceMm = hauteurMm + U(-30,+30)
```
Les échantillons alimentent `DensiteEstimator::ajouterEchantillon` puis
`estimer(now)` ; la salle reçoit `densite = est.surface`, `regime`, `confiance`,
`nbPersonnesEstime`.
Hauteur par défaut : 210 cm (2100 mm) ou hauteur mesurée × 10.

### 4.8 Anticipation
`Salle::mettreAJourAnticipation()` — régression linéaire sur les 120 derniers points
de `occHist` : `penteTendance = pente × 60` (pers/min). Si occupation ≥ capacité →
`anticipationMin = 0` ; si pente ≤ 0.05 → `-1` (jamais) ; sinon
`anticipationMin = ceil(restant / pente)`.

### 4.9 LED/LCD simulés
`Salle::majAffichageLedLcd()` calcule la cible LED (couleur + mode `normal`/`stroboscope`)
et les 2 lignes LCD (16 caractères, justifiées). Sur changement :
- LED : `ledCouleurConfirmee = ledCouleur` (rétroaction simulée) + log « LED démo » ;
- LCD : log « LCD démo » avec les 2 lignes.

### 4.10 Divers
- `creerSalle` : `enLigne=true`, `enAttente=false`, `occupation=0`, crée les 3 moteurs
  (estimateur avec hauteur mesurée ou 210 cm, détecteur d'intrusion avec horaires,
  machine A-B branchée), log « SALLE créée ».
- `getHauteurPorte` : après 650 ms, `valeur = 205 + U(0,21)` cm, propage à l'estimateur,
  émet `hauteurPorteMesuree(id, valeur, true, "Mesure démo confirmée")`.
- `supprimerSalle` : retire salle, accumulateurs, flags ToF, cibles de flux et les 3 moteurs.
- `majDecisionsFlux` : voir §7 (logs « FLUX UNI — …redirection vers X » /
  « FLUX MULTI — …attente ~N min »).

---

## 5. La structure Salle (models/Salle.h)

Champs d'état : `id, nom, groupeId, modeFlux, capacite (30), horaireDebut ("07:00"),
horaireFin ("22:00"), seuilEvacuation (95), hauteurPorteCm (-1), hauteurPorteMesuree,
enLigne, enAttente, occupation (-1), densite, nbPersonnesEstime, tendance, penteTendance,
anticipationMin, dernierHeartbeatMs, uptimeS, nbEntrees, nbSorties, regime, confiance,
evacuationActive, intrusionActive, intrusionDureeS, lcdLigne1/2, ledCouleur, ledCouleurConfirmee,
ledMode, decisionFlux, redirectionVers, attenteEstimeeMin`.

Historiques RAM (plafond 1800 points) : `occHist, densHist, entHist, sortHist, timeHist,
fluxSortieHist` (600 points pour F3).

Méthodes clés :
- `taux()` = occupation/capacité (0 si invalide).
- `majDetectionFluxSortie(debit, now, fenetreSec=600, minPoints=60, facteurSigma=3.0, minDebit=5.0)`
  → alerte latched μ+3σ (voir §4.5).
- `mettreAJourAnticipation()` (voir §4.8).
- `pushHistorique(ts)` : pousse les 5 séries horodatées.
- `occupationTexte()` : `"occ/cap"` ou `"--/--"`.
- `statutTexte()` : EN ATTENTE DE CONFIRMATION / HORS LIGNE / EVACUATION ACTIVE /
  INTRUSION ACTIVE / EN LIGNE.
- `couleurLed()` : vert < 0.60, jaune ≥ 0.60, orange ≥ 0.80, rouge ≥ 0.95 ; rouge si
  évacuation/intrusion ; éteint si attente/hors ligne.
- `majAffichageLedLcd()` → `ChangementAffichage{ledChanged, lcdChanged}` (voir §4.9).

---

## 6. Les moteurs (engine/)

### 6.1 DensiteEstimator (engine/densite)
- Entrée : `ajouterEchantillon(distanceMm, tMs)` ; buffer **60 échantillons** (12 s à 5 Hz).
- **Filtrage** : exige ≥ 3 échantillons, **médiane des 3 derniers** puis **EMA
  (0.6·EMA + 0.4·valeur)** ; rejette `target_status != 0`.
- **closeness** : `0` si `d >= seuilPresenceMm`, sinon `clamp((seuil - d) / 800.0, 0, 1)` ;
  surface = moyenne sur la fenêtre 3000 ms.
- **Clusters** : « run » = présence continue (`d < seuil`) ; validé si durée ≥ 200 ms,
  plongeon `(hauteur - min) ≥ 150 mm`, fin ≤ 3000 ms, écart entre clusters ≥ 600 ms.
- **Régimes** (hystérésis 3000 ms) :
  - clustering → surface si `nbClusters >= 3` ou (`nbClusters <= 1` et `surface >= 0.45`) ;
  - surface → clustering si `surface <= 0.30`.
- **Personnes** : surface → `max(round(table(surface)), nbClusters)`, confiance 0.90
  (calibré) sinon 0.65 ; clustering → `nbClusters`, confiance 0.85 (≥ 1 cluster) sinon 0.55.
- **Calibration F18** : table par défaut `{(0.00→0.0), (0.10→0.4), (0.20→1.0), (0.35→2.0),
  (0.50→3.0), (0.65→4.5), (0.80→6.5), (1.00→10.0)}` ; fichier `calibration_<id>.json`
  (tableaux `surface`/`personnes`) chargé s'il existe ; interpolation linéaire par
  morceaux avec clamp.
- Hauteur porte par défaut 2100 mm ; `setHauteurPorteCm(h)` → `seuilPresenceMm = h - 800`.

### 6.2 PassageDetectorAB (engine/passage)
Machine à états `Attente / VuA / VuB`, `fenetreMs = 5000`, `debounce = 2` :
- `majToF(bloque)` : front ToF validé après 2 échantillons consécutifs ; en `Attente`
  → `VuA` (`capteurAActive`) ; en `VuB` → retour `Attente` + `passageValide("sortie")`.
- `declencherUltrason(ts)` : en `Attente` → `VuB` (`capteurBActive`) ; en `VuA` →
  retour `Attente` + `passageValide("entree")`.
- `verifierExpiration(now)` : état ≠ Attente et > 5000 ms → `Attente` + `sequenceAnnulee()`.
- Signaux : `passageValide(direction)`, `capteurAActive()`, `capteurBActive()`, `sequenceAnnulee()`.
- Géométrie : **A = VL53L0X/ToF devant la porte (extérieur)** ; **B = HC-SR04/ultrason
  derrière (intérieur)**. A→B = entrée, B→A = sortie.

### 6.3 IntrusionDetector (engine/securite)
- `setHoraires(debut, fin)` (défaut 07:00-22:00, `HH:mm`) ; horaires inversés =
  plage traversant minuit ; `debut == fin` = jamais hors horaire.
- `verifier(presence, now)` → `IntrusionResultat{horsHoraire, intrusionActive,
  nouvelleAlerte, dureeS}` : présence hors horaire chronométrée ; alerte quand
  `dureeS >= 120.0` (latched jusqu'à retour dans les horaires/absence).

### 6.4 FluxOrchestrator (engine/flux)
- `calculer(salles, groupes, dernieresCibles*, seuilSaturation = 0.95)` → `DecisionFlux
  {decision, redirectionVers, attenteEstimeeMin}`.
- Salle ignorée si `occupation < 0`, hors ligne ou en attente ; si `taux < 0.95` → normal.
- **UNI** : garde la cible en cours tant qu'elle est valide (anti ping-pong) ; sinon
  cherche la porte membre dont `tauxAutre <= taux - seuilEcart` et taux minimal →
  `"redirection"` vers cette porte.
- **MULTI / indépendant / UNI sans alternative** : `"attente"` avec
  `attenteEstimeeMin = 1.0 / rSortieMoyen` (r = moyenne des 180 derniers `fluxSortieHist`,
  K = 1 personne) ; `-1.0` (indéterminée) si `r <= 0.1`.

---

## 7. Décisions de flux et affichage

- `majDecisionsFlux()` (démo : fin de chaque tick ; MQTT : chaque tick du watchdog) :
  recalcule les décisions, compare à l'état précédent (décision, cible, attente à
  0.01 près), applique et logue sur changement.
- En MQTT, `publierDecisionFlux` envoie `{"mode": ...}` (+ `"vers"` si redirection,
  + `"attente_min": max(1, round(attente))` si attente) sur `salle/<id>/flux/decision`.
- `publierAffichage` n'émet que si LED ou LCD ont changé :
  `salle/<id>/led/set` `{"couleur", "luminosite": 80}` et `salle/<id>/lcd/set`
  `{"ligne1", "ligne2"}`. Les retours `led/etat` et `lcd/etat` confirment côté ESP32.

---

## 8. Source MQTT réelle (MqttSource + MqttClient)

### 8.1 Abonnements (QoS 0) et champs JSON reçus
| Topic | Champs JSON |
|---|---|
| `salle/+/raw/tof` | `d_mm` (double, défaut -1), `status` (int, défaut 4), `t_ms` |
| `salle/+/raw/ultrason` | `event` = `"presence"` (→ `declencherB`) ; `"depart"` = info seule |
| `salle/+/raw/env` | réservé (ignoré) |
| `salle/+/heartbeat` | `etat` (`"online"`), `uptime_s` |
| `salle/+/etat` | `occupation`, `capacite`, `densite`, `en_ligne` |
| `salle/+/config/confirm` | `nom`, `capacite`, `horaires.{debut,fin}`, `hauteurPorte_cm` |
| `salle/+/led/etat` | `couleur`, `mode` |
| `salle/+/lcd/etat` | `ligne1`, `ligne2` |

### 8.2 Publications
| Topic | Payload |
|---|---|
| `salle/<id>/etat/get` | `{}` (sur `actualiserSalle`) |
| `salle/<id>/config/set` | `{nom, capacite, seuilEvacuation, hauteurPorte_cm, horaires:{debut,fin}}` |
| `salle/<id>/flux/decision` | `{mode, [vers], [attente_min]}` |
| `salle/<id>/led/set` | `{couleur, luminosite: 80}` |
| `salle/<id>/lcd/set` | `{ligne1, ligne2}` |

### 8.3 Pipeline ToF → A-B → passages
- `raw/tof` : ToF **bloqué** si `status != 4 && d_mm > 0 && d_mm/10 < hauteur - 20`
  (hauteur = mesurée, sinon 210 cm, marge 20 cm sous le linteau) → `majEtatToF`
  → `PassageDetectorAB::majToF`.
- `raw/ultrason` avec `event == "presence"` → `PassageDetectorAB::declencherUltrason`.
- Passage validé : `"entree"` → `occupation+1` ; `"sortie"` → `occupation-1` +
  horodatage dans `m_departTimes` + `verifierFluxSortie`. Signaux `etatAB` pour
  chaque transition (attente/vu_a/vu_b), `passageValide(salleId, direction, ts)`.
- `verifierExpirationAB` chaque seconde (watchdog) : annule les séquences > 5 s.

### 8.4 Watchdog (toutes les secondes)
1. expiration A-B ; 2. publication affichage si changement ; 3. **heartbeat > 30 s** →
   `enLigne=false` + log « HORS LIGNE… aucun heartbeat depuis 30 s » ; 4. si en ligne
   et occupation ≥ 0 : anticipation, pushHistorique, `verifierFluxSortie`,
   `verifierIntrusion`, estimation densité ; 5. `majDecisionsFlux()`.

### 8.5 Mesure de hauteur de porte (MQTT)
- `getHauteurPorte` : timer unique **3500 ms** ; collecte des trames `raw/tof`
  (`status != 4 && d_mm > 0`) tant que `id == m_mesureId` ; **7 échantillons** requis →
  `finaliserMesure(true)` ; sinon échec. Résultat = **médiane** des mm ÷ 10 (cm),
  signal `hauteurPorteMesuree(id, cm, succes, note)`.

### 8.6 MqttClient (MQTT 3.1.1 maison sur QTcpSocket)
- États : `Disconnected / Connecting / Connected` ; keep-alive **60 s**, PINGREQ
  toutes les **30 s** ; CONNECT niveau 4 (3.1.1), clean session ; gestion CONNACK,
  PUBLISH (QoS 0/1/2 décodés), PINGRESP, SUBACK ; encodage « remaining length » ;
  `publish(topic, payload, qos, retain)` ; déconnexion propre (DISCONNECT 0xE0 0x00).
- Signaux : `stateChanged`, `errorMessage`, `messageReceived(topic, payload)`.

---

## 9. Historique (HistoryManager) — détail

- **Agrégation à la minute** : `PendingSample` par salle (bucket = ts/60000) ; à chaque
  minute (`onMinute`), les buckets terminés sont figés : occupation = moyenne arrondie,
  densité/fluxSortie/tendance/confiance = moyennes, compteurs = dernières valeurs,
  `observations` = nb de points. Écriture CSV en append avec en-tête si fichier vide.
- **Passages** : buffer de 64 événements avant écriture (lots).
- **Alertes** : `alertes_history.csv` réécrit intégralement via `QSaveFile` (commit
  atomique) à chaque ajout/modification.
- **Lecture** : fichiers relus à la demande, mis en cache (empreinte taille+mtime),
  filtrés par période ; le sample du bucket courant est ajouté en mémoire.
- **Exports** : `exportSalleCsv` (lignes `echantillon`/`passage`/`alerte`),
  `exportAlertesCsv`.
- CSV : encodage UTF-8, échappement des guillemets (doublés), locales C (point
  décimal).

---

## 10. Harnais de test SM_SHOT (main.cpp)

Activé par la variable d'environnement `SM_SHOT` (ex. `SM_SHOT=1 ./salles_manager`).

### Étape 0 (t = 2,5 s) — Cartes de démo
Création de 5 salles factices dans la grille (`B201` Amphi A 82/120, `B202` Labo Info
21/30 en attente, `B203` TD Maths 28/30, `B204` Salle Réunion 6/12 hors ligne,
`B205` Salle Évacuée 130/130 évacuation). Test de style : coupe du QSS à
« CARTES DE SALLE », captures `card_before.png` / `card_after.png` (fond vert forcé),
puis capture globale `app.png`.

### Étape 1 (t ≈ 4,2 s) — testAnticipation
Cherche des ids de scénario via `chercherIdScenario` :
```cpp
for (i = depart; i < depart + nbEssais; ++i)
    if (int(qHash("A%1".arg(i, 3, 10, '0'))) % 6 == scenario) return candidat;
```
Crée 3 salles sur un `DemoSource` :
- scénario **1** (montée rapide) — « Amphi Anticipation », capacité 30 ;
- scénario **4** (sortie brusque) — « Amphi Sortie Brusque », capacité 30 ;
- scénario **5** (présence permanente) — « Amphi Intrusion », horaires **00:00–00:01**
  (hors horaires presque toute la journée).

Ouvre la carte de la salle d'anticipation + « Afficher la courbe ».

### Étape 2 (t ≈ 30 s)
Lecture de `occupation/cap/penteTendance/anticipationMin/densHist` ; capture
`detail_anticipation.png`, `config_reseau.png`.

### Étape 3 (t ≈ 84 s) — alerte F3
Vérifie `fluxSortieAnormal` + points d'histogramme de la salle scénario 4, compte des
alertes `flux_sortie` dans `AlerteModel` ; captures `detail_alerte.png`,
`alertes_panel.png`. Puis suppression de la salle : clic carte → « Supprimer la salle »
→ confirmation `QMessageBox` (capture `suppr_confirmation.png`, clic Yes) → vérifie
retrait du modèle et de la carte, capture `suppr_apres.png`.

### Étape 4 (t ≈ 130 s) — intrusion F11
Vérifie `intrusionActive/dureeS/occupation` de la salle scénario 5 et le compte des
alertes `intrusion` ; capture `intrusion.png`. Puis `testStade`.

### Étape 5 (t ≈ 130 s + 55 s) — Stade UNI-MARKET
Crée le groupe `STADE1` « Stade Sud » `ModeFlux::Uni`, `seuilEcart = 0.15`, avec
3 portes (capacités 4/4/10) : `porte1` = scénario 1 (départ 401), `porte2` = scénario 1
(départ 801), `porte3` = scénario 3 (départ 1201, reste vide).
Vérifie : décisions (`p1/p2` redirection vers porte vide, attente), l'absence de
cartes des portes dans la grille MULTI, la présence de la `stadeCard`, ouverture du
dialogue stade (capture `stade_interface.png`), comptage des `stadeSalleCard`,
ouverture de l'éditeur de la porte 1 (capture `stade_porte_edition.png`,
`stade_grille.png`) puis quitte l'application.

Les captures sont écrites dans `/tmp/opencode/`.

---

## 11. Compilation et tests

```bash
cmake -S salles_manager -B salles_manager/build -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_PREFIX_PATH=/home/fabien/.Qt/6.11.1/gcc_64
cmake --build salles_manager/build --parallel 2
# démo avec harnais de test :
QT_QPA_PLATFORM=offscreen SM_SHOT=1 salles_manager/build/salles_manager
```

- Qt 6 (Core, Gui, Widgets, Network, PrintSupport), C++17, AUTOMOC/AUTORCC,
  QCustomPlot en vendored source (`../vendor/qcustomplot.{h,cpp}`).
- Test : `history_smoke` (`tests/HistoryManagerSmoke.cpp`) — vérifie le cycle
  record/flush/relecture/export de l'historique. Enregistré via `enable_testing()` +
  `add_test` (CTest).

---

## 12. Constantes utiles (aide-mémoire)

| Élément | Valeur |
|---|---|
| Tick de simulation (démo) | 1000 ms |
| Fréquence ToF simulée | 5 Hz (échantillon toutes les 200 ms) |
| Probabilité de présence ToF (démo) | `occupation / 3`, borné [0,1] |
| Plongeon ToF (démo) | `900 + min(occ,10)×60` mm ; bruit ±100 mm actif, ±30 mm libre |
| Hauteur de porte par défaut | 210 cm (démo et MQTT) |
| Mesure hauteur (démo) | 205–225 cm après 650 ms |
| Mesure hauteur (MQTT) | 7 échantillons / 3,5 s / médiane |
| Watchdog MQTT | 1000 ms ; hors ligne après 30 s sans heartbeat |
| Machine A-B | fenêtre 5000 ms ; debounce 2 ; marge hauteur 20 cm |
| Intrusion | ≥ 120 s de présence hors horaires |
| Saturation | seuil 0.95 ; LED jaune ≥ 0.60, orange ≥ 0.80, rouge ≥ 0.95 |
| F3 (flux sortie) | μ + 3σ, min 5 pers/min, fenêtre 600 s, min 60 pts, latched |
| Anticipation | régression linéaire, fenêtre 120 pts, pente ×60, plancher 0.05 |
| Historique RAM | 1800 points par série ; fluxSortieHist 600 pts |
| Historique disque | agrégat 1 min ; passages par lots de 64 |
| Orchestrateur | K = 1 personne ; R_sortie = moyenne 180 pts ; seuilEcart groupe 0.15 |
| MQTT keep-alive | 60 s ; PINGREQ 30 s |
| Luminosité LED commandée | 80 |
| Densité | buffer 60 éch. ; fenêtres 3000 ms ; seuil présence `hauteur-800` mm |
| Calibration densité | `calibration_<id>.json` ou table par défaut (8 points) |
| Alertes | types : evacuation, bousculade, saturation, immobile, intrusion, flux_sortie |

---

## 13. Signaux DataSource (contrat d'interface)

`salleAjoutee(id)`, `salleSupprimee(id)`, `salleMiseAJour(id)`,
`groupeAjoute(id)`, `groupeSupprime(id)`, `groupeMiseAJour(id)`,
`hauteurPorteMesuree(id, centimetres, succes, note)`,
`statutSource(connecte, note)`, `logAppend(ligne)`, `erreur(message)`,
`passageValide(salleId, direction, timestampMs)`, `etatAB(salleId, etat, timestampMs)`,
`alerte(const Alerte&)`.
