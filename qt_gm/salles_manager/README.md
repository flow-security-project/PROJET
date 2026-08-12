# Gestionnaire de salles

Cette cible est le point d'entrée de la gestion des salles. Elle possède la
source de données et intègre directement la vue détaillée et QCustomPlot.
`qt_gm` n'est plus lancé par ce parcours.

## Fonctionnalités

- Création manuelle d'une salle avec identifiant unique.
- Mesure de la hauteur de porte via les trames ToF MQTT existantes.
- Simulation complète de la mesure et de l'état en mode démo.
- Cartes cliquables en grille responsive.
- Modification, actualisation et masquage d'une carte.
- Réaffichage explicite des cartes masquées.
- Vue détaillée intégrée avec occupation, débit, compteurs et historiques.
- Courbe QCustomPlot alimentée directement par la source active.
- Source MQTT avec statut d'attente de confirmation.
- Historique persistant par salle : séries agrégées à la minute, passages
  horodatés et alertes rechargées au démarrage.
- Chargement des historiques Jour/Semaine/Mois et exports CSV/PDF depuis le
  détail d'une salle ; export CSV global depuis le panneau d'alertes.

## Stockage de l'historique

Les fichiers sont enregistrés dans le répertoire Qt
`QStandardPaths::AppDataLocation/history` :

- `salle_<id>_history.csv` : un point agrégé par minute ;
- `salle_<id>_passages.csv` : passages individuels validés, avec horodatage ;
- `alertes_history.csv` : alertes, statut d'acquittement et métadonnées d'appel.

Le buffer RAM glissant reste utilisé pour la vue temps réel. Les périodes
Jour/Semaine/Mois relisent les séries CSV à la demande.

Les lectures répétées d'un même fichier sont servies par un cache mémoire
invalidé automatiquement lorsque le fichier change. Les passages sont écrits
par lots pour limiter les ouvertures de fichiers lors d'un flux important.

## Compilation autonome

Depuis la racine du projet :

```bash
cmake -S salles_manager -B salles_manager/build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=/home/fabien/.Qt/6.11.1/gcc_64
cmake --build salles_manager/build --parallel 2
```

## Lancement

```bash
QT_QPA_PLATFORM=offscreen salles_manager/build/salles_manager
```

Pour une utilisation graphique normale, retirer `QT_QPA_PLATFORM=offscreen`.
