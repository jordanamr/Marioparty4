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

## Rejouer et accélérer un test sur une seule machine

Depuis Linux, la campagne lance le jeu Linux natif et le jeu Windows x64 sous Wine. Chaque paire reçoit un port UDP local, deux affichages Xvfb privés et des profils de jeu indépendants. Les prérequis sont Python 3.10+, Xvfb, Wine, un GPU compatible Vulkan, un préfixe Wine capable de démarrer le jeu Windows avec Direct3D 12 et le runtime Visual C++, les deux paquets de la même révision et un disque du jeu. Le préfixe fourni est copié pour chaque paire. Le disque reste sur la machine.

Pour rejouer l’entrée au deuxième tour de Koopa avec Windows hôte, placer les chemins locaux dans cette commande. Choisir un dossier de résultats neuf à chaque lancement. Omettre `--replay-input` pour piloter les menus avec le parcours intégré. La commande `--list-scenarios` liste les parcours disponibles.

```sh
python3 tools/netplay_cross_sweep.py \
  --appimage /chemin/PartyBoard.AppImage --windows-zip /chemin/PartyBoard.zip \
  --rom /chemin/disque.rvz --wine-prefix /chemin/prefixe-wine \
  --output essais/koopa-01 --package-cache build/paquets-test \
  --scenario koopa --windows-host --fast --trace-hashes \
  --replay-input /chemin/Rejeu-Koopa-PAL.txt
```

Le mode `--fast` supprime l’attente de présentation, désactive la synchronisation verticale et utilise une petite surface. Il exécute chaque pas du jeu avec ses rappels de dessin, sa logique de plateau et ses contrôles réseau. Le jeu doit confirmer ce mode des deux côtés. Ajouter `--mixed-graphics` à `--fast` pour garder Linux en ultralarge à l’échelle 1× et Windows en 4:3 à l’échelle 3×. Plusieurs options `--scenario` avec `--jobs 2` lancent deux paires en parallèle. Le débit dépend du parcours, du pilote et de la contention GPU.

Le fichier `manifest.json` indique si chaque paire a atteint le tour demandé ou l’écran suivant les résultats. En cas de désaccord, les journaux des deux processus conservent la première trame divergente et, quand les rapports détaillés sont présents, le premier champ en désaccord. Pour comparer la cadence normale et la cadence accélérée sur les mêmes entrées, activer `--trace-hashes` dans les deux campagnes, puis comparer les traces jusqu’à la dernière trame exigée, incluse.

```sh
python3 tools/netplay_compare.py essais/normal/linux.hashes essais/rapide/linux.hashes \
  --hash-traces --through 6000
```

Le lecteur de bandes filtre historiquement le bouton START pendant les mini-jeux. Une sortie de mini-jeu par ce bouton demande un essai en direct.

Les chemins des traces figurent dans le résultat du scénario. Le comparateur vérifie la continuité des trames depuis zéro. Les rapports détaillés d’un désaccord Windows/Linux se comparent avec la même commande sans les options de trace.

Pour essayer plusieurs décisions autour d’un événement, prendre la bande `inputs.txt` du scénario et modifier ses entrées sur une fenêtre précise. Ici deux variantes changent une commande du joueur 0 entre les trames 10 560 et 10 799. Le manifeste du corpus donne la graine et les modifications. Passer son répertoire à `--replay-dir` avec un seul `--scenario`.

```sh
python3 tools/generate_input.py variants inputs.txt --window 10560:10800 \
  --seed 19 --seat 0 --hold 6 --count 2 -o essais/variantes
```

Les régressions ciblées sont également exécutables sans ROM, fenêtre ni GPU avec `tools/test_minigame_determinism.py`. L’option `--jobs 8` parallélise les compilations et les essais. Ce niveau explore systématiquement les données des fonctions isolées, comme les 40 000 répartitions de passagers et d’occupants du train de Boo. Les replays complets permettent de vérifier ensuite les interactions entre ces fonctions.

## Rejouer le catalogue

Avec les mêmes arguments de paquets, de ROM et de préfixe Wine que ci-dessus, donner à chaque campagne son propre répertoire `--output`.

| Parcours | Options du pilote |
| --- | --- |
| Les 51 mini-jeux du jeu libre, depuis le menu en français | `--free-play-all --fast --trace-hashes` |
| Les dix autres jeux, dont les extras et les événements | `--special-all --fast --trace-hashes` |
| Boo jusqu’au quatrième tour, Windows hôte, réglages vidéo différents | `--scenario boo --windows-host --board-turn 4 --fast --mixed-graphics --trace-hashes` |

Les jeux libres utilisent les commandes des deux manettes pour rejoindre la partie depuis le titre. Les jeux spéciaux utilisent les points d’entrée de test prévus pour leurs événements. Pour un plateau, `board_turn_checkpoints` dans `case.json` indique l’image où les deux machines ont confirmé chaque tour. Un essai arrêté au quatrième tour ne couvre que les événements rencontrés sur sa bande.

Pour travailler sur la sélection des personnages, générer les trois bandes avec `python3 tools/netplay_character_select.py --output essais/bandes`. Ajouter `--scenario toad --board-turn 1 --replay-dir essais/bandes` au pilote. L’arrivée sur le plateau confirme que les deux joueurs ont franchi la validation des personnages et des bots. Les options `--windows-host` et `--linux-host` permettent de rejouer les mêmes gestes dans les deux sens.

Le pilote partage un budget GPU entre les campagnes du même utilisateur. Une paire en mode accéléré compact occupe une place, une paire avec les deux profils graphiques en occupe deux. Ajuster `PARTYBOARD_QA_GPU_SLOTS` à la mémoire vidéo disponible en conservant la même valeur pour les campagnes simultanées. `--jobs` limite le nombre de paires dans une seule campagne.
