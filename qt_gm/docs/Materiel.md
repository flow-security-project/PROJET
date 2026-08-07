### Matériels Connectés à l'ESP32 (Version Finale Corrigée)

| Composant | Référence | Interface | Rôle |
| :--- | :--- | :--- | :--- |
| Capteur ToF | VL53L0X | I2C | Détection passage / estimation densité |
| Capteur Ultrason | HC-SR04 | GPIO | Validation directionnelle A-B / anti-rebond |
| Écran LCD | 16x2 I2C | I2C | Affichage local statut / occupation |
| LED RGB | WS2812B Ring 12px | GPIO | Signalisation visuelle progressive |
| Microphone MEMS | INMP441 | I2S | Signature acoustique anomalie |
| Capteur T°/HR | SHT4x | I2C | Confirmation contexte incendie |

> **Précisions :**
> -   Le système A-B (double capteur entrée/sortie) est réalisé par le couple **VL53L0X + HC-SR04** positionnés de part et d'autre de la porte. La séquence temporelle entre les deux signaux valide la direction et rejette les faux positifs.
> -   Aucun stockage interne ou externe sur le boîtier. Toutes les données sont transmises en temps réel via MQTT. Le buffer et l'historisation sont gérés côté backend/dashboard Qt.
> -   L'audio est géré exclusivement au niveau de l'ordinateur central (dashboard Qt + Asterisk).
