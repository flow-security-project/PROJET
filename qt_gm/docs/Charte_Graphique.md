Voici une charte graphique **Corporate / Aerospace Engineering** stricte, conçue pour Qt Creator. Elle rejette explicitement les tendances "AI/Modern UI" (gradients, coins arrondis excessifs, fond sombre par défaut) au profit d'une esthétique fonctionnelle, lisible et professionnelle inspirée des interfaces de contrôle SpaceX Dragon et des systèmes industriels critiques.

---

## Charte Graphique i++ v4.0 — Style Corporate Engineering

### 1. Philosophie Visuelle

| Principe | Application | À Éviter Absolument |
| :--- | :--- | :--- |
| **Fonction > Forme** | Chaque pixel sert l'information. Pas de décoration gratuite. | Ombres portées, effets glassmorphism, animations décoratives |
| **Lisibilité Critique** | Contraste élevé, hiérarchie claire, densité informationnelle maîtrisée | Gradients texte/fond, polices fines <12px, transparence |
| **Précision Technique** | Alignements stricts, grille visible, bordures nettes | Coins arrondis >2px, espacements irréguliers, flou |
| **Neutralité Professionnelle** | Palette sobre, couleur = information uniquement | Couleurs saturées décoratives, thèmes personnalisés fantaisistes |
| **Cohérence Système** | Identique sur tous écrans/résolutions, imprimable en N&B | Responsive adaptatif excessif, dépendance GPU |

---

### 2. Palette de Couleurs (Mode Clair Obligatoire)

#### Fond & Structure
| Rôle | Code Hex | Usage |
| :--- | :--- | :--- |
| Fond principal | `#FFFFFF` | Background global fenêtre, onglets, panneaux |
| Fond secondaire | `#F5F5F5` | Zones regroupement, headers tableau, barre état |
| Bordure structurelle | `#D0D0D0` | Séparateurs, contours widgets, grille |
| Bordure active/focus | `#4A90D9` | Widget sélectionné, champ focus, bouton actif |

#### Texte & Information
| Rôle | Code Hex | Usage |
| :--- | :--- | :--- |
| Texte primaire | `#1A1A1A` | Titres, données critiques, labels principaux |
| Texte secondaire | `#555555` | Descriptions, unités, métadonnées, timestamps |
| Texte désactivé | `#999999` | Champs non éditables, boutons inactifs |
| Texte inversé | `#FFFFFF` | Uniquement sur badges/alertes colorés pleins |

#### Couleur Sémantique (Information Uniquement)
| État | Code Hex | Usage Strict |
| :--- | :--- | :--- |
| Normal / Sécurisé | `#2E7D32` | LED verte, statut OK, validation |
| Attention / Prédiction | `#F57C00` | LED orange, anticipation, warning |
| Critique / Alerte | `#C62828` | LED rouge, évacuation, erreur, intrusion |
| Informatif / Neutre | `#1565C0` | Liens, sélection, indicateur neutre |
| Hors service / Dégradé | `#616161` | Nœud offline, capteur HS, mode dégradé |

> ⚠️ **Règle absolue** : Aucune couleur sémantique n'est utilisée comme élément décoratif. Le rouge n'apparaît QUE pour signaler un état critique. Jamais de bouton rouge "stylé". Jamais de header coloré.

---

### 3. Typographie — Style SpaceX / Engineering

#### Police Principale
-   **Famille** : `Inter` (priorité) ou `Roboto Mono` pour données techniques
-   **Fallback** : `Segoe UI` → `Helvetica Neue` → `Arial`
-   **Licence** : Open Source (SIL OFL), libre usage commercial/académique

#### Hiérarchie Typographique Stricte
| Niveau | Taille | Poids | Usage | Espacement |
| :--- | :--- | :--- | :--- | :--- |
| Titre Fenêtre | 18px | SemiBold (600) | Nom application, titre zone principale | Margin bottom 16px |
| Titre Section | 14px | SemiBold (600) | Onglets, headers panneau, nom salle | Margin bottom 8px |
| Label / Donnée | 13px | Regular (400) | Contenu standard, valeurs capteurs | Line-height 1.4 |
| Métadonnée | 11px | Regular (400) | Timestamps, unités, descriptions secondaires | Couleur #555555 |
| Monospace Données | 13px | Mono Regular | Valeurs numériques, topics MQTT, codes erreur | Roboto Mono obligatoire |

> 💡 **Pourquoi pas SpaceX Starship font ?** La police officielle SpaceX est propriétaire. Inter + Roboto Mono reproduisent fidèlement l'esthétique technique aérospatiale tout en étant libres de droits et parfaitement rendues sur tous OS. C'est le choix engineering pragmatique.

---

### 4. Composants Qt — Spécifications Visuelles

#### Boutons
```css
QPushButton {
    background-color: #FFFFFF;
    border: 1px solid #D0D0D0;
    border-radius: 2px;           /* Quasi-carré, pas rond */
    padding: 6px 16px;
    font-size: 13px;
    font-weight: 500;
    color: #1A1A1A;
}
QPushButton:hover {
    border-color: #4A90D9;        /* Seul feedback hover : bordure bleue */
    background-color: #FAFAFA;    /* Changement subtil, pas gradient */
}
QPushButton:pressed {
    background-color: #EEEEEE;
}
QPushButton:disabled {
    color: #999999;
    border-color: #E0E0E0;
}
/* Bouton critique UNIQUEMENT pour évacuation */
QPushButton#btnEvacuation {
    background-color: #C62828;
    color: #FFFFFF;
    border: 1px solid #B71C1C;
    font-weight: 600;
}
```

#### Tableaux & Listes
```css
QTableWidget, QListView {
    background-color: #FFFFFF;
    border: 1px solid #D0D0D0;
    gridline-color: #E8E8E8;      /* Grille très subtile */
    font-size: 13px;
    selection-background-color: #E3F2FD;  /* Sélection bleu pâle, pas saturé */
    selection-color: #1A1A1A;     /* Texte reste sombre même sélectionné */
}
QHeaderView::section {
    background-color: #F5F5F5;
    border-bottom: 2px solid #D0D0D0;  /* Header souligné, pas rempli */
    padding: 6px 8px;
    font-weight: 600;
    font-size: 12px;
    color: #555555;               /* Header moins contrasté que données */
}
```

#### Champs de Formulaire
```css
QLineEdit, QSpinBox, QTimeEdit {
    border: 1px solid #D0D0D0;
    border-radius: 2px;
    padding: 5px 8px;
    font-size: 13px;
    background-color: #FFFFFF;
}
QLineEdit:focus, QSpinBox:focus {
    border-color: #4A90D9;        /* Focus = bordure bleue nette */
}
QLineEdit:read-only {
    background-color: #F5F5F5;
    color: #555555;
}
```

#### Barres de Progression & Indicateurs
```css
QProgressBar {
    border: 1px solid #D0D0D0;
    border-radius: 2px;
    background-color: #F5F5F5;
    text-align: center;
    font-size: 11px;
    height: 18px;                 /* Hauteur fixe, compacte */
}
QProgressBar::chunk {
    background-color: #1565C0;    /* Bleu informatif par défaut */
    border-radius: 1px;
}
/* Variantes sémantiques via setStyleSheet dynamique */
/* Saturation proche → chunk #F57C00 */
/* Évacuation active → chunk #C62828 + animation clignotante CSS */
```

#### Badges & Alertes
```css
/* Badge plein, pas outline, pas gradient */
QLabel#badgeCritique {
    background-color: #C62828;
    color: #FFFFFF;
    padding: 2px 8px;
    border-radius: 2px;
    font-size: 11px;
    font-weight: 600;
}
QLabel#badgeAttention {
    background-color: #F57C00;
    color: #FFFFFF;
    /* Même structure, seule couleur change */
}
```

---

### 5. Règles d'Or Anti-AI / Pro-Corporate

| ✅ FAIRE | ❌ NE JAMAIS FAIRE |
| :--- | :--- |
| Bordures 1px nettes, coins ≤2px | Bordures épaisses, coins arrondis >4px, ombres |
| Fond blanc/gris clair uniforme | Fonds dégradés, textures, motifs, dark mode par défaut |
| Couleur = information sémantique stricte | Couleur décorative, accent gratuit, rainbow palette |
| Typographie hiérarchisée par taille/poids | Tailles aléatoires, polices display/script, italique abusif |
| Espacement régulier basé sur grille 4px | Marges asymétriques, padding excessif, whitespace décoratif |
| Icônes linéaires monochromes 16x16 | Icônes remplies colorées, illustrations, emojis |
| Données alignées à droite (numériques) | Centrage numérique, justification texte données |
| Feedback interaction discret (bordure/couleur plate) | Animations bounce/elastic, transitions longues, particules |
| Impression N&B parfaite | Dépendance couleur pour compréhension information |
| Cohérence absolue entre tous widgets | Styles custom par widget, incohérences visuelles |

---

### 6. Application au Prototype i++

| Zone Interface | Traitement Charte Corporate |
| :--- | :--- |
| Barre état global | Fond #F5F5F5, séparateur bas 1px #D0D0D0, texte 12px #555555, indicateurs cercles pleins 8px couleur sémantique |
| Grille supervision | Cards blanches bordure 1px #D0D0D0, padding 12px, nom salle 14px SemiBold, occupation 24px Mono Bold, LED cercle 12px plein couleur sémantique |
| Courbe QCustomPlot | Fond blanc, grille #E8E8E8 pointillés, axes #555555, courbe A-B #1565C0 2px, aire densité #F57C00 20% opacité, légende 11px coin supérieur gauche |
| Conditions évacuation | Barres fond #F5F5F5, chunk couleur sémantique, label droite 12px Mono, checkmark ✓ vert #2E7D32 quand seuil atteint |
| Panneau alertes | Lignes alternance blanc/#FAFAFA, séparateurs 1px #E8E8E8, badge gauche 4px largeur pleine hauteur couleur sévérité, timestamp 11px Mono #555555 |
| Formulaire config | Labels gauche alignés droite 13px #555555, champs largeur uniforme, bouton envoi bas droite style standard, statut confirmation 11px sous bouton |
| Reflet LCD | Cadre #D0D0D0 2px, fond #1A1A1A, texte #2E7D32 monospace 14px (simulation LCD réel), pas d'effet glow/néon |

Cette charte garantit une interface **professionnelle, lisible en conditions opérationnelles, imprimable, et durable visuellement**. Elle signale immédiatement au jury un projet d'ingénierie sérieux, pas un exercice de design trend. Souhaitez-vous que je génère le fichier `.qss` complet prêt à intégrer dans votre projet Qt Creator ?
