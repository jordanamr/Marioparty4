# Réglages vidéo et déterminisme en ligne sur PC

## Désynchronisation de Take a Breather

Deux rapports de la même session sur un disque européen GMPP01 révision 0 situent le premier désaccord à l’image 12052, dans le mini-jeu 55 (module m456, contexte 62). Les entrées et les compteurs d’images correspondent. La comparaison des champs capturés à l’image du désaccord donne les valeurs suivantes.

| Champ | Linux | Windows |
| --- | --- | --- |
| Tirages de frand depuis le démarrage réseau | 42 374 | 42 478 |
| État de frand | b5cac5b6 | f31c96a6 |
| Hauteur de cinq modèles, en hexadécimal IEEE 754 | 41cb7c7a | 41caca4e |

Le dessin de l’eau demandait deux tirages pour chacune des 26 paires de coordonnées de texture. Deux dessins supplémentaires consommaient donc 104 tirages. L’ancienne AppImage et l’ancien exécutable Windows ont aussi divergé au cours d’une autre partie automatisée du même mini-jeu. Pour comparer deux rapports, utiliser les lignes FIELD de l’image concernée. La ligne de résumé RNG peut avoir été recueillie quelques images plus tard.

Sur PC, la simulation actualise désormais les coordonnées de l’eau et la durée des remous. Le dessin lit ces valeurs. Les paramètres aléatoires des reflets sont préparés à la création de l’effet. Les modules m442 et m455 contenaient des dépendances similaires. Le moteur de particules distingue aussi les passages des caméras et des ombres pour éviter d’actualiser plusieurs fois une particule pendant le même pas de jeu.

## Réglages de chaque joueur

L’échelle de résolution, le format 4:3 ou ultralarge, le plein écran et l’habillage de l’interface sont locaux. La cible de rendu en ligne reste à 60 images par seconde sur les deux machines. Le réglage de cadence pour le solo reste disponible. Les deux joueurs doivent utiliser les exécutables de la même révision du fork.

## Autres chemins vérifiés

- Une file de dessin fixe pouvait dépasser sa capacité de 512 objets selon les éléments visibles. Le port PC adapte sa capacité et conserve la file de la caméra lorsqu’un effet dessine une autre passe. Un test du code de tri et de dispatch a vérifié l’ordre de 2 003 objets, avec deux niveaux de passes imbriquées, sur Linux et avec un exécutable Windows sous Wine.
- Le générateur de nombres sur huit bits utilise explicitement l’arithmétique modulo 2³² de la console. Les mini-jeux qui combinent deux tirages pour former une valeur de seize bits choisissent l’octet de poids fort en premier sur les deux compilateurs.
- L’audit a identifié des lectures en dehors des tableaux de modèles ou de mouvements, deux transformations de quatre sommets qui supposaient que des variables indépendantes étaient contiguës sur la pile, des entrées manette indéfinies et des retours de fonction manquants. Les corrections visent les valeurs et les types effectivement attendus par les appelants.
- Les rapports de désynchronisation conservent désormais les adresses de retour sur toute la largeur d’un pointeur. Cela permet d’identifier le module appelant sur les exécutables 64 bits, lorsque le module est encore chargé.

Une compilation et un parcours de jeu ne couvrent pas à eux seuls tous les embranchements d’un plateau. Pour comparer les comportements après une modification, reprendre le même RVZ sur les deux pairs, choisir des réglages opposés et comparer les états confirmés jusqu’au retour du mini-jeu ou jusqu’après la sortie de la boutique.

## Curseurs et fenêtres de la salle Extra

Dans le module m409, un essai à deux clients Linux a identifié à l’image 8165 une différence de huit blocs de mémoire système, soit 5 024 octets. Les deux rapports donnent les mêmes entrées et le même état des générateurs. Le recensement des allocations permet de remonter au viseur, dont le rappel de dessin pouvait détruire une instance avant le prochain pas de jeu. Les modules m409 et m415 exécutent désormais cette mise à jour depuis le gestionnaire des objets. La sonde de régression vérifie la libération des huit blocs avec des cadences de dessin différentes.

Le menu des jeux bonus comparait les dimensions de ses textes à des valeurs non initialisées. De plus, le calcul de la mémoire du fond de fenêtre tronquait les dimensions alignées à huit bits. Le menu part désormais d’une taille nulle et le moteur réserve toute la surface des tuiles. Sur Linux comme sur Windows, le compagnon peut conserver le journal des états confirmés et son fichier détaillant les désaccords.

Depuis la racine du projet, après compilation du port Linux avec Ninja, la commande suivante exécute les régressions sur l’eau, les textures et la durée de vie du viseur, sans image disque.

```sh
python3 tools/test_minigame_determinism.py build/linux-clang-relwithdebinfo
```
