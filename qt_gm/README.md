# i++ Qt

Application Qt de supervision des flux et des salles.

## Organisation

- `salles_manager/` : application active de création, configuration et supervision.
- `vendor/` : dépendance QCustomPlot partagée.
- `firmware/` : code de test ESP32 et protocole MQTT.
- `docs/` : documentation fonctionnelle, matériel et architecture.
- `legacy/qt_gm/` : ancienne application Qt conservée pour référence et compatibilité.
- `reference/qt_dashboard/` : ancienne implémentation conservée comme référence de conception.

Les dossiers `legacy/` et `reference/` ne sont pas nécessaires à l'exécution de
`salles_manager`.

## Compilation de l'application active

Depuis la racine du projet :

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=/home/fabien/.Qt/6.11.1/gcc_64
cmake --build build --target salles_manager --parallel 2
```

L'exécutable est généré dans `build/salles_manager/salles_manager`.

## Compilation de l'ancienne application

Elle est désactivée par défaut :

```bash
cmake -S . -B build-legacy \
  -DBUILD_LEGACY_QT_GM=ON \
  -DCMAKE_PREFIX_PATH=/home/fabien/.Qt/6.11.1/gcc_64
cmake --build build-legacy --target qt_gm --parallel 2
```
