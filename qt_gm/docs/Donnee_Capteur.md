Voici le mapping complet et précis entre chaque capteur, les données brutes capturées, le traitement embarqué sur l'ESP32, et les signaux MQTT envoyés au backend. Ce tableau est la **spécification technique d'interface** de votre projet.

---

### Données Capturées & Signaux Envoyés par Capteur

| Capteur | Donnée Brute Capturée | Traitement Embarqué ESP32 | Signal MQTT Envoyé (Topic / Payload) | Fréquence |
| :--- | :--- | :--- | :--- | :--- |
| **VL53L0X** (ToF) | Distance mm (zone centrale ou moyenne 4x4) + target_status | • Filtrage médian 3 trames<br>• Calcul surface occupée (zones < seuil_porte)<br>• Clustering spatial simplifié<br>• Bascule régime densité | `salle/{id}/flux`<br>`{"distance_mm": 820, "surface_m2": 0.34, "clusters": 2, "regime": "clustering", "density": 0.45}` | 5 Hz |
| **HC-SR04** (Ultrason) | Temps écho µs → distance cm | • Validation directionnelle avec VL53L0X (séquence temporelle)<br>• Rejet faux positifs (stationnement > 30s)<br>• Comptage entrée/sortie validé | `salle/{id}/passage`<br>`{"direction": "entree", "timestamp_ms": 1705312987000, "validation": "AB_confirm"}` | Événementiel |
| **INMP441** (Micro MEMS) | Échantillons audio I2S 16kHz | • FFT bande 200-4000Hz<br>• Calcul RMS glissant 1s<br>• Détection bande cris 800-3000Hz<br>• Percentile_99 dynamique | `salle/{id}/audio`<br>`{"rms": 0.72, "band_cris": true, "percentile99": 0.68}` | 1 Hz |
| **SHT4x** (T°/HR) | Température °C + Humidité %RH | • Calcul dérivée dT/dt sur fenêtre 30s<br>• Compensation humidité<br>• Détection gradient rapide | `salle/{id}/environnement`<br>`{"temp_c": 28.4, "hr_pct": 62, "dTdt_Cmin": 2.3}` | 0.5 Hz |
| **LED RGB WS2812B** | — (actionneur) | Réception commande MQTT → mise à jour couleur/mode | `salle/{id}/led/etat` (feedback)<br>`{"couleur": "orange", "mode": "progressif", "luminosite": 80}` | Sur changement |
| **LCD 16x2** | — (actionneur) | Réception commande MQTT → affichage texte | `salle/{id}/lcd/etat` (feedback)<br>`{"ligne1": "Occ: 24/30", "ligne2": "En ligne"}` | Sur changement |

---

### Signaux MQTT Synthétiques (Calculés par ESP32)

Ces signaux ne proviennent pas d'un capteur unique mais sont le résultat de la **fusion/traitement embarqué** :

| Signal Synthétique | Sources | Traitement ESP32 | Topic MQTT / Payload | Usage Backend |
| :--- | :--- | :--- | :--- | :--- |
| **État global salle** | VL53L0X + HC-SR04 + Config capacité | Fusion occupation + anticipation + mode évacuation | `salle/{id}/etat`<br>`{"occupation": 24, "capacite": 30, "densite": 0.80, "anticipation_min": 8, "evacuation": false, "en_ligne": true}` | Dashboard Qt Zone 2+3, Asterisk |
| **Score bousculade** | VL53L0X + HC-SR04 + INMP441 + SHT4x | Corrélation temporelle ≥2 capteurs, score normalisé [0-1] | `salle/{id}/alerte/bousculade`<br>`{"score": 0.94, "capteurs_actifs": ["audio","surface"], "confiance": "haute"}` | Alerte Qt Zone 4, Appel Asterisk F10 |
| **Condition évacuation** | INMP441 + SHT4x + VL53L0X | Vérification ≥2/3 critères pendant 3s | `salle/{id}/alerte/evacuation`<br>`{"active": true, "criteres": {"audio": true, "thermique": false, "surface": true}, "debut_ts": 1705312987}` | Mode évacuation F9, Stroboscope LED, LCD, Asterisk |
| **Personne immobilisée** | VL53L0X + HC-SR04 | Variance ≈0 + occupation stable >5min | `salle/{id}/alerte/immobile`<br>`{"active": true, "duree_s": 312, "occupation": 1}` | Alerte Qt, Appel infirmerie F12 |
| **Intrusion hors horaires** | HC-SR04 + VL53L0X + Horloge interne | Présence >2min hors planning | `salle/{id}/alerte/intrusion`<br>`{"active": true, "duree_s": 145, "horaire": "03:17"}` | Alerte Qt, Appel agent sécu F11 |

---

### Messages de Contrôle (Backend → ESP32)

| Commande Backend | Topic MQTT | Payload | Action ESP32 |
| :--- | :--- | :--- | :--- |
| Configuration salle | `salle/{id}/config/set` | `{"nom":"B204","capacite":30,"horaires":{"debut":"07:00","fin":"22:00"}}` | Mise à jour paramètres RAM + confirmation |
| Forcer mode évacuation | `salle/{id}/evacuation/force` | `{"active": true}` | Override automatique, stroboscope immédiat |
| Reset alerte | `salle/{id}/alerte/reset` | `{"type": "bousculade"}` | Acquittement local, LED retour normal |
| Demande état | `salle/{id}/etat/get` | `{}` | Réponse immédiate payload état global |
| Test LED/LCD | `salle/{id}/test` | `{"composant": "led", "valeur": "rouge"}` | Diagnostic maintenance |

---

### Points Clés pour le Jury

1.  **Pas de donnée brute non traitée envoyée** : L'ESP32 ne publie jamais de valeur capteur brute sans filtrage/validation. Tout signal MQTT est une **information sémantique** exploitable directement par le backend.
2.  **Fréquences adaptées au besoin** : Audio 1Hz (suffisant pour RMS), environnement 0.5Hz (inertie thermique), flux 5Hz (réactivité passage), événements passage/événementiel (pas de polling inutile).
3.  **Feedback actionneurs** : LED et LCD renvoient leur état réel via MQTT. Le dashboard Qt affiche toujours l'état **confirmé**, pas l'état commandé. Cohérence garantie.
4.  **Signaux synthétiques = valeur ajoutée** : Le backend reçoit des scores, des conditions, des états fusionnés. L'intelligence est **embarquée**, pas déportée. C'est exactement ce qui distingue un projet MISA d'un simple relais de capteurs.
5.  **Topics structurés hiérarchiquement** : `salle/{id}/categorie/signal`. Permet subscription wildcard (`salle/+/etat`) pour supervision globale et subscription précise (`salle/B204/alerte/bousculade`) pour traitement spécifique. Scalable nativement.

Ce mapping est la **colonne vertébrale technique** de votre projet. Il démontre la maîtrise complète de la chaîne acquisition → traitement → communication → supervision. Souhaitez-vous que je l'intègre dans le document `.md` final comme annexe technique ?
