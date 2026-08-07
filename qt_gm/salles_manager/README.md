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
