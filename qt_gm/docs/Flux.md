# i++ v4.0 — Système Distribué de Gestion de Flux et de Sécurité pour ERP

> Système embarqué de supervision de flux piétonnier, de détection d'anomalies et d'alerte vocale centralisée, conçu pour le déploiement dans les Établissements Recevant du Public (universités, grands bâtiments). Intégration PBX Asterisk pour notifications téléphoniques et guidage vocal. Audio entièrement centralisé sur le poste de supervision.

**Auteurs :** ANDRIANANTENAINA Fabien Toky Fandresena, RAKOTONDRASOA Joharimisa  
**Établissement :** Université d'Antananarivo — Mention MIT (MISA)

---

## Table des matières

- [Vue d'ensemble](#vue-densemble)
- [Matériel Embarqué par Nœud](#matériel-embarqué-par-nœud)
- [Fonctionnalités de Base](#fonctionnalités-de-base)
- [Fonctionnalités Avancées Sécurité](#fonctionnalités-avancées-sécurité)
- [Intégration Asterisk & Guidage Vocal Centralisé](#intégration-asterisk--guidage-vocal-centralisé)
- [Gestion Multi-Personnes : Estimation de Densité](#gestion-multi-personnes--estimation-de-densité)
- [Architecture Système](#architecture-système)
- [Stack Technologique](#stack-technologique)
- [Originalité et Valeur Académique](#originalité-et-valeur-académique)

---

## Vue d'ensemble

Le projet **i++** est un système de sécurité active destiné aux ERP. Chaque nœud embarqué est autonome pour la détection locale et la signalisation visuelle, tandis que l'intelligence décisionnelle, l'audio et la supervision sont centralisés sur le poste Qt/Asterisk. Le système est conçu selon les principes de sûreté de fonctionnement : mode dégradé sécurisé, intégrité cryptographique des communications, et défaillance sûre.

---

## Matériel Embarqué par Nœud

| Composant | Référence | Interface | Rôle |
| :--- | :--- | :--- | :--- |
| Capteur ToF | VL53L0X | I2C | Détection passage / estimation densité |
| Capteur Ultrason | HC-SR04 | GPIO | Validation directionnelle A-B / anti-rebond |
| Écran LCD | 16x2 I2C | I2C | Affichage local statut / occupation |
| LED RGB | WS2812B Ring 12px | GPIO | Signalisation visuelle progressive |
| Microphone MEMS | INMP441 | I2S | Signature acoustique anomalie |
| Capteur T°/HR | SHT4x | I2C | Confirmation contexte incendie |

> **Notes importantes :**
> - Aucun stockage interne ou externe sur le boîtier. Toutes les données sont transmises en temps réel via MQTT. Le buffer et l'historisation sont gérés côté backend/dashboard Qt.
> - L'audio (guidage vocal, alertes sonores) est géré exclusivement au niveau de l'ordinateur central via Asterisk/TTS. Aucun buzzer ni module audio embarqué.
> - Le système A-B (double capteur entrée/sortie) est réalisé par le couple VL53L0X + HC-SR04 positionnés de part et d'autre de la porte.

---

## Fonctionnalités de Base

### 1 — LED Progressive
La LED WS2812B évolue progressivement selon le taux d'occupation réel : Vert (faible) → Jaune (modérée) → Orange (proche max) → Rouge clignotant (capacité atteinte). Lecture immédiate sans consulter le dashboard.

### 2 — Anticipation de la Saturation
Calcul de la vitesse d'évolution de l'occupation `(entrées/min − sorties/min)`. Si tendance > 0 et salle proche capacité max → alerte anticipée avant saturation réelle. Le système devient prédictif.

### 3 — Détection de Flux de Sortie Anormal
Hausse brutale du flux de sortie détectée par le système A-B → déclenchement automatique alerte sécurité. Le comptage devient outil de sécurité.

### 4 — Historique des Entrées/Sorties
Chaque passage horodaté et conservé côté backend. Traçabilité complète jour/semaine/mois avec export CSV/PDF depuis le dashboard Qt.

### 5 — Courbe d'Évolution
Dashboard Qt affiche courbe temporelle occupation temps réel via QCustomPlot. Visualisation tendances, pics, heures creuses.

### 6 — Supervision Multi-Salles Simultanée
- Vue d'ensemble grille avec statut couleur
- Fenêtre détaillée par salle (historique + courbes)
- Détection panne explicite : nœud hors ligne signalé "NON FIABLE"

### 7 — Configuration par Salle
Configuration individuelle depuis dashboard Qt (nom, capacité max, horaires). Transmis au boîtier via MQTT sans reprogrammation firmware.

### 8 — Écran LCD Local
LCD 16x2 affiche localement : statut boîtier (en ligne/hors ligne), état capteurs, occupation actuelle / capacité max. Lecture immédiate sans poste central.

---

## Fonctionnalités Avancées Sécurité

### 9 — Détection d'Intrusion Hors Horaires
Présence détectée hors planning autorisé par système A-B → alerte intrusion confirmée après temporisation anti-rebond algorithmique. Appel Asterisk vers agent de sécurité avec localisation précise. L'intrusion est définie uniquement par présence hors horaires autorisés (NFC supprimé).
---

## Intégration Asterisk & Guidage Vocal Centralisé

### Rôle d'Asterisk
Passerelle voix/SIP sécurisée, jamais exposée directement au réseau IoT. Backend communique via API ARI. **Toute synthèse vocale et diffusion audio sont centralisées sur l'ordinateur du dashboard Qt. Aucun module audio ni buzzer sur les boîtiers.**

### Scénarios d'Appel Automatique

| Type d'Alerte | Définition Algorithmique | Destinataire | Message TTS Généré |
| :--- | :--- | :--- | :--- |
| Saturation critique | ≥ 95% capacité > 3 min continu | Gestionnaire bâtiment | *"Saturation critique amphithéâtre A, 312/300 personnes depuis 4 minutes"* |
| Intrusion hors horaires | Occupation > 0 hors planning > 2 min | Agent surveillance | *"Intrusion non autorisée salle TP3, heure 03h17"* |
| Nœud suspect réseau | > 5 échecs HMAC/min OU valeurs impossibles | Admin IT | *"Anomalie cybersécurité nœud Salle-C12, perte intégrité données"* |

### Guidage Vocal Centralisé
Guidage vocal émis **exclusivement depuis l'ordinateur central** via Asterisk/TTS ou haut-parleurs dashboard Qt. Boîtiers assurent uniquement signalisation visuelle (LED + LCD). Architecture garantissant qualité audio professionnelle, maintenance simplifiée et accessibilité PMR sans surcoût matériel par nœud.

---

## Gestion Multi-Personnes : Estimation de Densité

### Principe Fondamental
Le VL53L0X ne compte pas des personnes en flux dense. Il mesure la surface projetée occupée dans son champ de vision. Cette surface est corrélée au nombre de personnes via un modèle calibré empiriquement.

### Logique de Bascule Automatique
- **Régime bas (1–2 personnes)** : Clustering spatial sépare silhouettes distinctes. Comptage individuel fiable.
- **Régime haut (≥ 3 personnes)** : Silhouettes se recouvrent, clustering échoue. Bascule vers estimation par surface occupée calibrée.

Bascule déclenchée lorsque nombre de clusters < moitié zones actives.

### Calibration Empirique Obligatoire
Courbe correspondance surface → nombre estimée construite in situ pour chaque installation :
1. Observateur humain compte personnes toutes les 5s pendant 30min flux varié (ground truth)
2. Capteur enregistre simultanément surface occupée horodatée
3. Régression polynomiale ajustée sur données appariées
4. Fonction convertie en table consultation légère embarquée ESP32

Calibration spécifique à chaque porte. Répétée après tout déplacement matériel.

### Précision et Limites Assumées

| Régime | Précision | Usage Valide |
| :--- | :--- | :--- |
| 1–2 personnes | ≥ 95 % | Comptage directionnel |
| ≥ 3 personnes | ± 15 % (MAPE) | Gestion saturation, alerte, prédiction |

Système jamais utilisé seul pour déclencher alerte. Estimation densité toujours corrélée aux autres signaux (acoustique, thermique, temporel).

> **Posture scientifique :** Instrument d'estimation densité pour gestion ERP. Pas compteur individuel flux dense. Limite documentée, quantifiée, acceptable pour cas d'usage cible.

---

## Stack Technologique

| Couche | Technologie | Justification |
| :--- | :--- | :--- |
| Firmware | C/C++ Arduino Framework ESP32 | Contrôle bas niveau, FSM déterministe |
| Communication | MQTT over TLS, QoS adaptatif | Fiabilité, sécurité, multi-topic natif |
| Backend | Java Spring Boot / Python FastAPI | Robustesse, intégration ARI Asterisk |
| PBX Vocal | Asterisk 20 + ARI | Standard ouvert, TTS, fiabilité éprouvée |
| Supervision | Qt 6 Creator C++ | Performance temps réel, widgets industriels |
| Visualisation | QCustomPlot | Courbes scientifiques temps réel |
| Audio Centralisé | Asterisk TTS + Qt AudioOutput | Qualité pro, maintenance centralisée |
