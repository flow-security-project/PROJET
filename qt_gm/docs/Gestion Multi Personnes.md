Gestion Multi-Personnes : Estimation de Densité par Surface Occupée
Principe Fondamental
Le VL53L5CX ne compte pas des personnes en flux dense. Il mesure la surface projetée occupée dans son champ de vision. Cette surface est corrélée au nombre de personnes via un modèle calibré empiriquement, car la relation n'est pas linéaire : trois personnes serrées occupent moins de surface que trois personnes espacées.

Logique de Bascule Automatique
Le système fonctionne en deux régimes distincts, avec bascule automatique :

Régime bas (1–2 personnes) : Le clustering spatial sépare les silhouettes distinctes. Comptage individuel fiable.
Régime haut (≥ 3 personnes) : Les silhouettes se recouvrent, le clustering échoue. Le système bascule vers l'estimation par surface occupée calibrée.
La bascule est déclenchée lorsque le nombre de clusters détectés devient inférieur à la moitié des zones actives, indiquant un recouvrement significatif.

Calibration Empirique Obligatoire
Une courbe de correspondance surface → nombre estimé est construite in situ pour chaque installation :

Un observateur humain compte les personnes toutes les 5 secondes pendant 30 minutes de flux varié (ground truth).
Le capteur enregistre simultanément la surface occupée horodatée.
Une régression polynomiale est ajustée sur les données appariées.
La fonction résultante est convertie en table de consultation légère embarquée sur l'ESP32-S3.
Cette calibration est spécifique à chaque porte (hauteur montage, largeur, revêtement sol). Elle doit être répétée après tout déplacement matériel.

Précision et Limites Assumées
Régime	Précision	Usage Valide
1–2 personnes	≥ 95 %	Comptage directionnel
≥ 3 personnes	± 15 % (MAPE)	Gestion saturation, alerte, prédiction
Le système n'est jamais utilisé seul pour déclencher une alerte. L'estimation de densité est toujours corrélée aux autres signaux (acoustique, thermique, temporel) pour confirmer les anomalies et éliminer les faux positifs.
