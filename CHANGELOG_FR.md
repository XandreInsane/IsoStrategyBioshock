## Version 0.7.0

- Correction : le clic sur une notification/alerte sélectionne désormais le vrai bâtiment concerné (l'ancien système devinait d'après le nom du type dans le texte et tombait toujours sur le même bâtiment).
- Nouveaux bâtiments : Forêt sous cloche (production de nourriture, 2x2) et Résidence de prestige (logement coûteux, moins d'occupants, bonus de moral), chacun avec sa propre technologie de déblocage (20 technologies au total).
- Obstacles océaniques : rochers et failles générés aléatoirement hors de la zone de départ, bloquent la construction ; nouvel onglet Océan pour repérer et envoyer une équipe les dégager (coût, délai, ouvriers mobilisés).
- Campagne étendue à 11 objectifs cohérents avec la progression (fondation, services, ouverture, expansion sous-marine, métropole, économie locale, progrès, recherche de pointe, classes sociales, utopie, transcendance).
- Sauvegarde version 10 (technologies, obstacles, chantier de dégagement en cours) ; les anciennes sauvegardes se chargent toujours (génération d'obstacles a posteriori).
- Rendu : nettoyage de blocs de code morts (doublons jamais exécutés) et nouvelle façade pour la centrale, le château d'eau, l'oxygénateur, la bathysphère, le métro, le sas, l'hôpital, la salle de fête, le recyclage et la sécurité (bandeaux muraux, hublots, signalétique), dans la continuité du labo et de la ferme déjà retouchés.

## Version 0.6.4

- **Lisibilite** : hausse des tailles de texte dans les infobulles, les listes, les onglets et les fiches ; objectifs de campagne et descriptions longues affiches sur plusieurs lignes dans leurs zones.
- **Ville** : infobulles explicatives au survol de chaque systeme vital et bouton « Tout reparer » pour lancer une maintenance groupee en fonction de la tresorerie disponible.
- **Listes** : indicateurs de defilement gris dans les listes de construction, de batiments, d'entreprises, de foyers et dans l'historique des annonces.
- **Actualites** : les annonces associees a un batiment peuvent etre cliquees dans la notification et dans l'historique pour selectionner ce batiment sur la carte.
- **Conduites en cuivre** : les tuyaux adjacents aux centrales, stations d'eau et unites d'oxygene augmentent leur production de 8 % chacun, jusqu'a 24 % ; les contours des cases sous les couloirs et tuyaux suivent maintenant l'option d'affichage de la grille.
- **Objectifs** : seuils de campagne et descriptions realistes et synchronises avec les conditions de progression reelles.
- **Verification** : tests de simulation et verification syntaxique des sources rendus valides ; le lancement complet avec raylib doit encore etre verifie dans l'environnement de compilation du jeu.

## Version 0.6.3

- Rendu : les bâtiments sont maintenant triés par un vrai test « derrière / devant » entre leurs empreintes (plus de laboratoire 2x1 dessiné par-dessus une résidence), les murs et plafonds des empreintes 2x2, 2x1 et 1x2 s'affichent (sens des triangles), les barres plates des bâtiments suivent les murs en perspective, le dôme de la ferme est posé sur le toit, le laboratoire porte une croix murale à la place du rond bleu.
- Interface : infobulle de construction avec la recherche requise, bouton NEWS (touche N) avec l'historique des annonces et de leurs effets, section ATTRACTIVITE dans l'onglet Ville (ce qui manque à la cité, mis à jour en direct).
- Recherche : 18 technologies sur 5 branches (Industrie, Science, Société, Commerce, Océan). Les Commerces dépendent maintenant de l'Économie de marché ; Arcadie, Héphaïstos et la Forteresse Folâtre demandent leur propre recherche.
- Démographie : les nouveaux foyers arrivent chaque jour selon l'attractivité de la ville (logement libre, emplois, satisfaction, services...). Les emplois sont répartis entre tous les employeurs au lieu d'aller tous à l'usine.
- Sauvegarde version 9 (18 technologies), lecture des versions précédentes conservée.

## Version 0.6.2

- Refonte du remplissage du prism multi-cases pour éviter les faces de mur ou portions de toit absentes sur les empreintes 2x2, 2x1 et 1x2.
- Ajout de la recherche « Commerces sous-marins », prérequis explicite pour Boutique, Restaurant et Cinéma.
- Sauvegarde version 8 et compatibilité de lecture des formats précédents.

# Journal des modifications

## Version 0.6.1

- **Faces arrière / gauche renforcées** : second remplissage des faces de mur à l'aide d'une triangulation complémentaire, puis réapplication du toit et de la façade gauche ; les détails de fenêtres sont redessinés après le remplissage afin d'éviter une surface vide sur les bâtiments 2 × 1, 1 × 2 et 2 × 2.
- **Catalogue réorganisé** : Corridor, Tuyau cuivre et Sécurité sont maintenant les trois premiers outils constructibles. Les descriptions de Sécurité et du Tuyau cuivre expliquent qu'ils ne remplacent pas le corridor : le tuyau est une conduite technique, Sécurité un bâtiment de service non traversable.
- **News plus lisibles** : notifications prolongées à 8 secondes au lieu de 3,5, avec une deuxième ligne décrivant l'effet concret sur le stock de matériaux, la trésorerie, l'état des bâtiments, le bonheur, les besoins vitaux ou les technologies.
- **Tests Release corrigés** : `rapture_sim_tests` est compilé avec les assertions actives (`-UNDEBUG` ou `/UNDEBUG`) même quand le type de build est Release. Cela évite que `assert()` soit silencieusement retiré et explique les avertissements de variables inutilisées observés précédemment.

## Version 0.6.0

- **Faces murales multi-cases corrigées** : les quatre murs sont remplis et ombrés avant le toit ; les deux faces arrière restent fermées même pour les empreintes allongées, les rotations et les 2 × 2.
- **Pêcherie-fontaine** : désormais remarquable, unique et de taille 2 × 2. La liste affiche explicitement la recherche d'aquaculture et l'accès maritime requis (sas ou bathysphère opérationnelle). Elle consomme eau et oxygène, produit beaucoup plus de nourriture et possède une fontaine turquoise distincte.
- **Arcadie** : remarquable et unique, 2 × 2 ; requiert trois fermes marines opérationnelles, produit beaucoup d'oxygène et augmente nettement la demande en eau. Dôme botanique vert et laiton.
- **Pôle d'Hephaïstos** : remarquable et unique, 2 × 2 ; requiert trois usines opérationnelles, apporte une forte production électrique et consomme jusqu'à six matériaux par mois. Silhouette industrielle cuivre/rouge avec double cheminée.
- **Forteresse Folâtre** : remarquable et unique, 2 × 2 ; requiert trois salles de fête opérationnelles, renforce fortement le moral/divertissement et consomme davantage d'énergie et d'oxygène. Palais violet à tourelles dorées.
- **Catalogue de construction clarifié** : au survol et au clic, les conditions manquantes sont précisées au lieu d'afficher un verrou générique. La liste indique qu'elle se parcourt à la molette.
- **Compatibilité** : format de sauvegarde version 7 pour identifier les nouveaux types ; les versions 1 à 6 restent acceptées. Une ancienne pêcherie 2 × 1 est migrée vers 2 × 2 au chargement si l'emprise additionnelle est libre, sinon l'ancienne ville est conservée sans chevauchement.
- **Tests** : ajout de cas sur les empreintes remarquables, accès maritime, prérequis, unicité et migrations de géométrie.

## Version 0.5.0

- **Alignement isométrique corrigé** : les emprises 2 × 1 et 2 × 2 sont construites à partir des quatre coins réels de la grille ; socles, murs, aperçu de placement et contours de sélection suivent la même projection. Les dimensions sauvegardées sont limitées à deux cases par axe.
- **Arbre scientifique repositionné** pour éviter le titre et les onglets. La nouvelle recherche « Sous-marins d’exploration » accélère le retour des expéditions à 6–9 jours (au lieu de 14–20) et réduit légèrement le risque.
- **Pêcherie-fontaine** : emprise 2 × 1 dans cette version ; nécessite l’aquaculture et un sas océanique opérationnel. Elle contribue à l’économie et aux réserves de nourriture.
- **Sauvegardes multi-parties** : cinq emplacements sélectionnables depuis `SAVE`/`LOAD`, `F5`/`F9` et le menu titre. L’emplacement 1 conserve `rapture_save.dat` ; les emplacements 2 à 5 ont leurs propres fichiers. Le format courant est la version 6 et le chargeur accepte les versions 1 à 5.
- **Commandes caméra et Échap** : le clic droit maintenu déplace la caméra ; raylib ne ferme plus la fenêtre sur Échap. La touche annule d’abord l’outil actif, puis la sélection, puis ouvre le menu.
- **Eau et oxygène plus importants** : la demande suit davantage la population ; les stations dépendent de l’électricité, les pénuries réduisent la satisfaction et freinent la production industrielle et alimentaire. Un rendement de secours limite les effets d’une courte panne.
- Tests supplémentaires pour l’emprise de la pêcherie, les prérequis du sas, les sous-marins, les sauvegardes et l’impact des services vitaux. Tests de simulation et vérifications mémoire passés lors de la validation de cette version.

## Version 0.4.0

- **Échap corrigé** : le premier appui annule l’outil de construction/démolition ; le suivant désélectionne l’objet ; lorsque rien n’est sélectionné, le menu de pause s’ouvre. Le menu permet de reprendre, sauvegarder ou retourner au titre.
- Ajout de rotation `R` pour les bâtiments de 2 × 1 cases, qui peuvent être placés horizontalement ou verticalement. Laboratoires et hôpitaux utilisent une emprise entière de deux cases, sans rendu de demi-case.
- Placement strict : l’emprise entière doit être libre, respecter les limites et être accolée orthogonalement à un corridor ou à un bâtiment existant.
- Génération du quartier initial variable : le cœur de services vitaux est garanti, tandis que l’ordre, l’emplacement et l’orientation de plusieurs bâtiments changent d’une nouvelle partie à l’autre. Les tests vérifient l’adjacence des ouvrages générés.
- Simulation sociale enrichie : indice de couverture sociale de chaque foyer, services culturels, restaurants, commerces et santé, affichage des besoins ou envies manquants, et effets modestes sur le bonheur afin de ne pas bouleverser la difficulté.
- Ajout des bâtiments cinéma, restaurant, boutique et pêcherie-fontaine. La pêcherie produit aussi de la nourriture ; les nouveaux services ajoutent des emplois et des revenus.
- Remplacement de la liste scientifique par un arbre de dix connaissances avec liens de prérequis, coûts, points de recherche et infobulle d’effet au survol.
- Événements aléatoires positifs et négatifs : incidents de corridor, mécontentement ouvrier, surtension, financement, récupération de matériaux et festivals.
- Notifications en jeu pour les alertes, les chantiers terminés, le retour des expéditions, la recherche et les événements importants. Les bâtiments très dégradés produisent une alerte précoce.
- Format de sauvegarde version 5 pour conserver les nouveaux états sociaux et les dix technologies ; chargement rétrocompatible avec les versions 1 à 4.
- Correction de l’appel non standard `DrawTextRec` : les descriptions d’infobulle utilisent désormais un rendu de texte avec retour à la ligne.
- Tests supplémentaires pour les villes initiales variables, les règles d’adjacence, l’emprise verticale des laboratoires et hôpitaux, les nouvelles entreprises et les prérequis de recherche.

## Version 0.3.0

- Raccordement automatique des corridors et tuyaux de cuivre selon les quatre voisins cardinaux : segments droits, coudes, fins de ligne, embranchements en T et intersections. Le motif se met à jour avec les constructions et démolitions voisines.
- Refonte des silhouettes des bâtiments pour faciliter l’identification sans clic : ferme marine sous dôme vitré, usine large avec deux cheminées et fumée animée, et volumes dédiés aux installations de recherche, de transport et de service.
- Empreintes de construction variables, chantiers progressifs, infobulle de construction et format de sauvegarde version 4.
- Correction de la sélection dans `ENTREPRISES` : sélectionner une société ne fait plus basculer le panneau vers `VILLE`.

## Version 0.2.0

- Inventaire des catégories de bâtiments, silhouettes Art déco, corridors et tuyaux de cuivre, caméra déplaçable et tutoriel interactif.
- Réglages fiscaux et tarifs de services publics.
- Expéditions océaniques pouvant être sauvegardées en cours de route.
- Extension des tests de simulation.

## Version 0.1.0

- Première version jouable en C17 avec raylib : ville isométrique, économie, entreprises, foyers, bâtiments et campagne.
