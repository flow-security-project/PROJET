Voici la liste complète, définitive et épurée des fonctionnalités du projet **i++ v4.0**, strictement alignée avec votre matériel réel (1 LED RGB + VL53L0X + HC-SR04 + LCD 16x2 + INMP441 + SHT4x), sans cybersécurité avancée ni mode dégradé complexe.

---

### Fonctionnalités de Base — ordre d'implémentation recommandé

L'ordre ci-dessous suit les dépendances techniques et l'importance opérationnelle. Les identifiants `F1` à `F8` sont conservés afin de ne pas modifier les références présentes dans les autres documents.

| Ordre | ID | Fonctionnalité | Description Technique |
| :--- | :--- | :--- | :--- |
| 1 | F8 | Écran LCD Local | LCD 16x2 affichant localement : statut réseau, état capteurs, occupation actuelle / capacité max. Reflet miroir dans Qt pour diagnostic distant. Cette fonction valide rapidement l'initialisation du nœud et fournit un retour local même sans dashboard. |
| 2 | F1 | LED RGB Progressive | La LED RGB unique change de couleur selon le taux d’occupation : Vert (<60%), Jaune (60-80%), Orange (80-95%), Rouge fixe (≥95%). Feedback visuel immédiat à la porte. |
| 3 | F6 | Supervision Multi-Salles | Grille de widgets dans Qt montrant toutes les salles avec code couleur identique à la LED RGB physique. Clic sur une salle ouvre le détail sans nouvelle fenêtre. Nœud hors ligne affiché explicitement "HORS LIGNE". Cette base permet de superviser et de valider les nœuds avant d'ajouter les traitements avancés. |
| 4 | F7 | Configuration Distante par Salle | Formulaire Qt pour définir nom, capacité max et horaires de chaque salle. Envoi via MQTT au boîtier concerné sans reprogrammation firmware. Cette fonction valide le canal de commande MQTT dans les deux sens. |
| 5 | F5 | Courbe d’Évolution Temporelle | Graphique QCustomPlot dans Qt affichant en temps réel la courbe d’occupation (comptage A-B) et la densité estimée. Scroll automatique, mise à jour 1Hz. Elle nécessite la réception et l'historisation des mesures. |
| 6 | F4 | Historique Entrées/Sorties | Chaque passage validé par le système A-B est horodaté et stocké côté backend. Export CSV/PDF disponible depuis le dashboard Qt. Cette fonction fournit la traçabilité nécessaire aux analyses et aux exports. |
| 7 | F3 | Détection Flux Sortie Anormal | Détection d’une hausse brutale du flux sortant par la séquence HC-SR04→VL53L0X dépassant μ+3σ de l’historique. Déclenche une alerte sécurité dans le panneau Qt et log horodaté. Elle dépend de la validation du système A-B et d'un historique suffisamment alimenté. |
| 8 | F2 | Anticipation de Saturation | Calcul de la tendance `(entrées/min − sorties/min)` via le système A-B. Si tendance positive ET occupation ≥80%, le dashboard Qt affiche une alerte prédictive avec timer "Saturation prévue dans X min". Cette fonction dépend du comptage directionnel, de l'historique et du calcul de tendance. |

---

### Fonctionnalités Avancées Sécurité

| # | Fonctionnalité | Description Technique |
| :--- | :--- | :--- |
| F11 | Détection Intrusion Hors Horaires | Présence détectée par le système A-B en dehors des plages horaires autorisées pendant >2 minutes → alerte intrusion. Appel Asterisk vers agent de sécurité avec localisation précise. |

---

### Intégration Asterisk & Guidage Vocal Centralisé

| # | Fonctionnalité | Description Technique |
| :--- | :--- | :--- |
| F13 | Appels Automatiques Structurés | Le backend communique avec Asterisk via l’API ARI pour 2 scénarios : saturation critique, intrusion hors horaires. Message TTS généré automatiquement. Statut de l’appel visible dans le panneau alertes Qt. |
| F14 | Guidage Vocal Centralisé | Toute la synthèse vocale et l’alarme sonore sont émises **exclusivement depuis l’ordinateur central** via Asterisk/TTS. Aucun module audio ni buzzer sur les boîtiers embarqués. Qualité professionnelle, maintenance simplifiée, accessibilité PMR. |
| F15 | Double Canal Humain/Machine | Chaque alerte Asterisk transmet un message vocal humain ET des données JSON structurées conservées côté backend pour analyse post-incident. Corrélation directe alerte ↔ appel dans Qt. |

---

### Gestion Multi-Personnes & Estimation de Densité

| # | Fonctionnalité | Description Technique |
| :--- | :--- | :--- |
| F16 | Système A-B Directionnel | Couple VL53L0X + HC-SR04 espacés de 40cm de part et d’autre de la porte. La séquence temporelle valide la direction entrée/sortie et rejette les faux positifs (stationnement, rebonds). Précision ≥97% en flux normal. |
| F17 | Estimation Densité par Surface Occupée | **Régime bas (1-2 personnes)** : clustering spatial VL53L0X pour comptage individuel fiable.<br>**Régime haut (≥3 personnes)** : bascule automatique vers estimation par surface occupée calibrée empiriquement in situ. Précision ±15% MAPE. |
| F19 | Transparence Métrologique dans Qt | La courbe F5 superpose le comptage A-B et la densité estimée. Un indicateur de régime (clustering/surface) informe l’opérateur de la fiabilité actuelle de la donnée. Score de confiance visible pour chaque alerte multimodale. |

---

### Résumé pour Présentation Jury

> **i++ v4.0 comporte 16 fonctionnalités** organisées en 4 axes :  
> 1. **Supervision opérationnelle** (F1-F8) : monitoring temps réel, configuration, historique  
> 2. **Sécurité active autonome** (F11) : détection d'intrusion hors horaires avec appel automatique  
> 3. **Alerte vocale centralisée** (F13-F15) : Asterisk intégré, guidage vocal PC-only, traçabilité appels  
> 4. **Intelligence de flux** (F16-F19) : système A-B directionnel, estimation densité calibrée, transparence métrologique  
>   
> **Matériel minimaliste** : 1 LED RGB + 5 capteurs/nœud. **Aucun bouton physique**. **Audio 100% centralisé**. Projet conçu comme système de supervision de flux et instrument de recherche pour ERP universitaires à budget contraint.

Cette version est propre, cohérente avec votre matériel réel, et prête à être intégrée dans votre rapport ou présentée au jury. Souhaitez-vous que je régénère le document Markdown complet avec ces 19 fonctionnalités ?
