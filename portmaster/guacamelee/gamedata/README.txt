Place the legally owned GOG installer here before the first launch:

`gog_guacamelee_gold_edition_2.0.0.3.sh`

The launcher extracts it with PortMaster's 7zzs utility and never executes the installer shell script. The extracted game data stays in this directory and is not included in the Git repository.

Required extracted files include `game-bin`, `resources.dat`, `media/`, and `lib32/libSDL2-2.0.so.0`.
