"""Full-game routes for minigames outside the standard Free Play lists.

Extra Room returns to mpexDll (75), its volleyball challenge returns to
ztardll (98), and a Chance Time visit returns to the player's board (w01, 89).
"""

SPECIAL_GAMES = {
    433: {"walk": "5:0", "results": (98,)},
    444: {"walk": "0:0", "results": (89,)},
    **{game: {"walk": "5:0", "results": (75,)}
       for game in (442, 451, 453, 459, 460, 461, 462, 463)},
}
