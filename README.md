# Rapture: Dream of the Deep — version 0.8.1

**City-builder isométrique en C17**, conçu autour d'une simulation de ville sous-marine Art déco. Tu incarnes Andrew Ryan : développe les quartiers, assure les services essentiels, fais progresser la recherche et dirige une économie dont les habitants et les entreprises réagissent à tes décisions.

Cette version contient une campagne en quatorze étapes, avec une progression dédiée à la découverte de l’ADAM, à la production des Petites Sœurs et à la fabrication des Protecteurs. Elle améliore les infobulles et la légende scientifique, affiche les effectifs débloqués dans l’onglet Foyer, ajoute des emblèmes pixel art intégrés aux façades et place des obstacles supplémentaires dans la zone intermédiaire de la carte. La sauvegarde version 11 conserve les ressources et les unités ADAM tout en restant compatible avec les versions précédentes.

## 1. Installation sous Windows avec MSYS2

Le chemin conseillé utilise **MSYS2 UCRT64**. Il faut utiliser ensemble le compilateur, CMake, Ninja et raylib de ce même environnement afin d'éviter les mélanges de bibliothèques Windows incompatibles.

### Étape A — installer les paquets

1. Ouvre le terminal **MSYS2 UCRT64** depuis le menu Démarrer.
2. Mets MSYS2 à jour :

   ```bash
   pacman -Syu
   ```

   Si MSYS2 demande de fermer le terminal, ferme-le, rouvre **MSYS2 UCRT64**, puis relance `pacman -Syu` jusqu'à ce que la mise à jour soit terminée.

3. Installe les outils et raylib :

   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-raylib
   ```

Tu n'as pas besoin de compiler raylib toi-même : le paquet MSYS2 fournit l'en-tête C et les bibliothèques nécessaires.

### Étape B — ouvrir le dossier

Décompresse l'archive, puis ouvre le terminal **MSYS2 UCRT64** dans le dossier contenant `CMakeLists.txt`. Par exemple :

```bash
cd /c/Users/natha/Documents/CodeC/Perso/Projet5
```

Remplace le chemin par celui de ton installation.

### Étape C — compiler et lancer

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/rapture.exe
```

Pour relancer le jeu plus tard, exécute simplement `./build/rapture.exe`. Lance-le depuis le terminal UCRT64 pour que Windows puisse trouver `libraylib.dll`.

### Étape D — tests de simulation

```bash
ctest --test-dir build --output-on-failure
```

Les tests couvrent la génération variable de la ville initiale, l'absence d'obstacles sous les bâtiments de départ, la construction et la démolition, les empreintes des nouveaux bâtiments, les prérequis scientifiques, les sauvegardes précédentes, la stabilité des cibles d'alerte, le déblaiement océanique et le cycle de vie des expéditions.

## 2. Contrôles

| Action | Commande |
|---|---|
| Choisir Campagne ou Bac à sable | Menu de démarrage |
| Déplacer la caméra | Maintenir le clic droit sur la carte et déplacer la souris |
| Déplacer la caméra au clavier | `W`, `A`, `S`, `D` ou flèches |
| Déplacer plus vite la caméra | `Maj` gauche + déplacement au clavier |
| Zoom | Molette de la souris sur la carte |
| Construire | Sélectionner un ouvrage dans le panneau gauche, puis cliquer sur une case libre |
| Construire plusieurs éléments identiques | Cliquer sur plusieurs cases successives |
| Annuler l’outil, désélectionner, puis ouvrir le menu | `Échap` (un appui par étape) |
| Annuler l’outil de construction/démolition | `Échap` |
| Tourner un bâtiment allongé de 2 × 1 à 1 × 2 | `R` pendant la pose |
| Ouvrir le menu de pause, sauvegarder ou revenir au titre | `Échap` quand rien n’est sélectionné (Échap ne ferme jamais la fenêtre) |
| Démolir | Bouton `DEMOLIR` ou touche `X`, puis cliquer sur un ouvrage |
| Afficher/masquer la grille | `G` |
| Pause / reprise | `Espace` ou bouton pause |
| Vitesse de simulation | Touches `1`, `2`, `3` ou boutons en haut |
| Sauvegarder | `F5` ou bouton `SAVE` |
| Charger | `F9` ou bouton `LOAD` |
| Nouvelle partie | Bouton `NEW`, puis choisir le mode |
| Naviguer dans l'inventaire des bâtiments | Molette sur la liste dans l'onglet `VILLE` |

À la vitesse x1, le jeu avance maintenant d'environ **un jour toutes les trois secondes**. Les vitesses x2 et x3 accélèrent proportionnellement le temps.

## 3. Modes de jeu

### Campagne

La campagne commence par un court tutoriel interactif en trois étapes. Il présente la carte, les principaux onglets, la construction, les besoins essentiels et les objectifs. Tu peux parcourir les pages, passer directement au jeu ou terminer le tutoriel avec les boutons affichés à l'écran.

Les six étapes de campagne guident le développement de la ville. Les objectifs sont consultables dans l'onglet `VILLE`.

### Bac à sable

Le bac à sable donne accès à une partie libre, sans tutoriel ni objectif final imposé. Tu peux développer la ville à ton rythme et ajuster les paramètres économiques.

## 4. Onglets et systèmes

### Ville

Quand aucun bâtiment n'est sélectionné, l'onglet `VILLE` affiche le bilan de la ville, les ratios de services vitaux, les paramètres fiscaux et **l'inventaire de tous les types de bâtiments avec leur quantité actuelle**. La liste défile à la molette. Quand un bâtiment est sélectionné, ses informations remplacent la liste d'inventaire.

Les paramètres ajustables sont :

- impôt sur le revenu des habitants (0 à 35 %) ;
- taxe sur les sociétés (0 à 20 %) ;
- taxe mensuelle par foyer (0 à 120 $) ;
- prix mensuel de l'électricité par habitant (0 à 12 $) ;
- prix mensuel de l'eau par habitant (0 à 12 $).

Ces paramètres affectent les recettes de la ville et les finances ou le bonheur des familles et des entreprises. Un tarif élevé peut améliorer les revenus publics, mais aussi peser sur les ménages et l'activité économique. La demande d'eau et d'oxygène augmente avec les habitants et les bâtiments ; une pénurie réduit le bonheur, et elle freine aussi la production des usines, fermes et pêcheries. Les centrales alimentent également les installations de traitement.

### Construction et infrastructures

La carte fait 48 × 48 cases. Les bâtiments ont des silhouettes dédiées pour être distingués sans les sélectionner : tours Art déco pour les logements, dôme de verre côtier pour la ferme marine, hall industriel large avec cheminées et fumée animée pour l'usine, formes techniques distinctes pour le laboratoire, les services vitaux et le transport. Les fenêtres suivent les deux faces visibles des volumes isométriques au lieu d'être dessinées comme des points de face. Les formes sont rendues par le moteur et ne demandent aucune texture à télécharger.

Les bâtiments n’occupent pas tous une seule case : l’usine et l’institut génétique utilisent une emprise de 2 × 2 cases ; plusieurs installations comme la ferme, le laboratoire, la centrale, le cinéma, le restaurant ou l’hôpital utilisent 2 × 1. Les quatre bâtiments remarquables — pêcherie-fontaine, Arcadie, Pôle d’Hephaïstos et Forteresse Folâtre — utilisent tous une emprise de 2 × 2. Appuie sur `R` pour tourner un bâtiment allongé et le placer sur 1 × 2 cases. Le socle, les parois, l’aperçu de pose et le contour de sélection utilisent les quatre coins projetés de l’emprise réelle ; les bâtiments multi-cases ne sont plus dessinés en agrandissant un losange au centre. Détruire un bâtiment depuis n’importe quelle case occupée le supprime entièrement.

Le placement est refusé si l’emprise est occupée, hors de la carte ou sans voisin cardinal déjà construit : un nouvel ouvrage doit toucher un corridor ou un autre bâtiment. Le quartier de départ garde un noyau de services et de corridors garantis, mais l’ordre et l’emplacement des ouvrages changent entre les nouvelles parties. Les corridors et les tuyaux en cuivre se raccordent automatiquement aux éléments voisins. Le rendu calcule les quatre sorties possibles : extrémité, ligne droite, coude, embranchement en T et intersection à quatre directions. Il suffit donc de poser les segments case par case ; le dessin des raccords se met à jour lorsqu'un voisin est ajouté ou supprimé.

Chaque ouvrage est un chantier avant de devenir opérationnel. Les corridors et tuyaux prennent 2 jours, la plupart des bâtiments 7 jours, les services vitaux majeurs 8 jours, et les grands bâtiments industriels ou génétiques 10 jours, la pêcherie-fontaine 12 jours et les bâtiments remarquables de 14 à 16 jours. Une armature, une barre de progression et une petite animation de chantier indiquent l'avancement. Le coût et les matériaux sont débités au démarrage, mais les installations en cours de construction ne produisent pas et ne fournissent pas leurs emplois avant la fin. Le panneau d'informations d'un bâtiment sélectionné indique aussi le temps restant.

Passe la souris sur un type de bâtiment dans le menu de gauche pour afficher sa fonction, son temps de construction, son emprise, son coût, ses matériaux et son entretien mensuel. La sélection d'une entreprise depuis l'onglet `ENTREPRISES` affiche son bâtiment sur la carte sans quitter cet onglet.

Les catégories comprennent les corridors, les tuyaux, les logements, le marché, l’usine, le laboratoire, l’énergie, l’eau, l’oxygène, la nourriture, l’hôpital, les loisirs, la sécurité, le cinéma, le restaurant, la boutique et la pêcherie-fontaine. La pêcherie-fontaine remarquable occupe 2 × 2 cases, se débloque par la technologie d’aquaculture et exige un sas ou une bathysphère opérationnelle. Arcadie nécessite trois fermes marines opérationnelles et produit beaucoup d’oxygène au prix d’une forte consommation d’eau ; le Pôle d’Hephaïstos nécessite trois usines, produit beaucoup d’électricité et consomme des matériaux chaque mois ; la Forteresse Folâtre nécessite trois salles de fête, consomme plus d’énergie et d’oxygène, et augmente fortement le divertissement. Chaque bâtiment remarquable ne peut être construit qu’une seule fois. Ils sont regroupés près des sas et bathysphères dans le catalogue pour rester visibles. Les bathysphères, le métro, les sas océaniques, le recyclage, la recherche génétique et les nouveaux services sociaux sont débloqués par les technologies.

### Foyers et société

La simulation suit les foyers, le nombre d’adultes et d’enfants, leur logement, les emplois, les revenus, l’épargne, les dépenses et le bonheur. Au-delà des besoins vitaux, le jeu estime les besoins sociaux : culture et loisirs, restaurants, commerces et santé. L’onglet `FOYERS` met en avant les services manquants et les envies probables des ménages (sorties, repas, commerces, emploi stable ou capacité d’épargne). Ces attentes pèsent progressivement sur le bonheur, sans rendre la ville impossible à gérer dès le départ.

### Entreprises et économie

Les entreprises ont des salariés, des salaires, des ventes, une trésorerie, des coûts d’exploitation et un risque de faillite. Les entrepreneurs PNJ peuvent créer de nouvelles sociétés. Cinémas, restaurants, boutiques et pêcheries-fontaines ajoutent des emplois et diversifient les services, tandis que la pêcherie contribue aussi aux stocks alimentaires. Les impôts et les dépenses des habitants influencent la demande et la viabilité des entreprises.

### Recherche scientifique

Les laboratoires génèrent des points de recherche. L’onglet `SCI.` montre un arbre de vingt-trois technologies reliées par leurs prérequis, dont la recherche de sous-marins d’exploration et la branche ADAM. Survole un nœud pour lire son coût, les connaissances nécessaires et l’effet attendu ; clique dessus pour lancer la recherche lorsque les prérequis, les points et la trésorerie sont suffisants.

### Expéditions océaniques

L'onglet `OCEAN` permet d'envoyer 2, 4, 6 ou 8 ouvriers récupérer des matériaux dans les profondeurs. Il faut disposer d'un accès à l'océan (bathysphère ou sas). Chaque expédition coûte de l'argent et comporte un risque d'incident. Sans sous-marins, le retour prend 14 à 20 jours ; la technologie `Sous-marins d'exploration` réduit ce délai à 6 à 9 jours et diminue aussi le risque. Une expédition réussie rapporte typiquement **30 à 104 matériaux** ; en cas d'incident, la ville perd de l'argent et l'équipe revient sans sa cargaison.

Les ouvriers envoyés en expédition ne sont temporairement plus disponibles pour les emplois locaux. Le jeu conserve une expédition active dans la sauvegarde et libère les ouvriers à son retour. Des notifications apparaissent à la fin d’un chantier, au retour d’une équipe, lors d’une alerte ou d’un événement important. Des incidents de corridor, des tensions ouvrières, des surtensions et aussi des bonnes nouvelles (investissements, matériaux récupérés, festivals) surviennent de manière aléatoire et restent volontairement modérés pour préserver l’équilibre.

## 5. Sauvegardes

Le bouton `SAVE` ou la touche `F5` ouvre le choix de cinq emplacements ; `LOAD` ou `F9` permet de choisir laquelle charger. L’emplacement 1 utilise `rapture_save.dat` pour reprendre les sauvegardes existantes ; les emplacements 2 à 5 utilisent `rapture_save_2.dat` à `rapture_save_5.dat`, dans le répertoire courant. Le format de sauvegarde passe en version 7 pour identifier les nouveaux types de bâtiments. Les formats de sauvegarde des versions 1 à 6 restent acceptés ; lorsqu’une ancienne pêcherie-fontaine 2 × 1 peut être agrandie sans chevaucher un autre bâtiment, son emprise est migrée vers 2 × 2 au chargement. Les emplacements sont indépendants : choisis le numéro que tu veux écraser lors d’une sauvegarde, puis ce même numéro pour reprendre la partie correspondante.

## 6. Structure du projet

```text
rapture_dream/
├── CMakeLists.txt
├── README_FR.md
├── VERSION.txt
├── launch_rapture.bat
├── src/
│   ├── main.c          # boucle principale et horloge de simulation
│   ├── rapture.h       # données, types de bâtiments et API
│   ├── simulation.c    # économie, foyers, entreprises, recherche, sauvegardes
│   ├── render.h        # état de l'interface
│   └── render.c        # rendu isométrique, contrôles et interface raylib
└── tests/
    └── test_simulation.c
```

## 7. Dépannage

- **`raylibConfig.cmake` introuvable** : vérifie que `mingw-w64-ucrt-x86_64-raylib` est installé et que tu utilises le terminal MSYS2 UCRT64.
- **`ninja` ou `cmake` introuvable** : installe les paquets UCRT64 listés plus haut et rouvre le terminal.
- **DLL manquante au lancement** : lance le jeu depuis le terminal UCRT64 pour que `C:\msys64\ucrt64\bin` soit accessible dans `PATH`.
- **Recompiler proprement** : supprime le dossier `build`, puis relance les commandes de configuration, de compilation et de lancement.
- **La trésorerie diminue** : vérifie les dépenses d'entretien, les entreprises en difficulté et les tarifs de services publics. Une fiscalité trop lourde peut réduire le bonheur et l'activité économique.

## Références techniques

- [Paquet raylib pour MSYS2 UCRT64](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-raylib)
- [Documentation MSYS2 : CMake et Ninja](https://www.msys2.org/docs/cmake/)
- [Dépôt officiel raylib](https://github.com/raysan5/raylib)
- [OpenRCT2 — référence d'inspiration pour la construction et le rendu isométriques](https://github.com/OpenRCT2/OpenRCT2)


## Correctifs 0.6.2

- Correction du rendu des empreintes multi-cases : faces murales remplies par quadrilatères complets, toit intégral et redessin des deux façades visibles après le toit.
- Ajout de la technologie dédiée **Commerces sous-marins**, qui débloque explicitement Boutiques, Restaurants et Cinémas.
- Sauvegarde version 8 ; chargement compatible avec les sauvegardes des versions 1 à 7.

## Branche ADAM (version 0.8.0 et suivantes)

La recherche « Découverte de l’ADAM » (après la recherche génétique) active la consommation d’ADAM par les habitants et ajoute cette ressource aux systèmes vitaux. La recherche « Production des Petites Sœurs » débloque les orphelinats, qui en forment lentement au prix d’une forte demande d’eau, d’oxygène et de nourriture. Les Petites Sœurs produisent l’ADAM ; les Instituts ADAM augmentent ce rendement. La recherche « Fabrication des Protecteurs » permet de construire des usines à Protecteurs, très gourmandes en énergie et en matériaux, qui consomment aussi un peu d’ADAM et renforcent la production des Petites Sœurs.

Les sauvegardes utilisent désormais le format 11 et conservent les ressources et unités de cette branche ; les anciens formats restent pris en charge.
