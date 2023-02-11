---
title: The state of gamepad support in Games
date: 2016-07-22 01:32:16
tags: [ GNOME, GSoC, Gamepad, Games, GNOME Games ]
categories: Opensource
---
Gamepad support has now been merged into [GNOME Games](https://wiki.gnome.org/Design/Playground/Games) [v3.21.4](https://github.com/Kekun/gnome-games/commit/c110c0390f40821779f2663bde50027b1f9f1acd) !!! This means that you can play your favorite retro games using a gamepad!!!

<!--more-->

## Which gamepads are supported?
But you may wondering which gamepads are supported out of the box. The answer is a [lot of them][1]! We use the [SDL mappings format][2] to map your gamepad to a standard gamepad (by this I mean a seventh generation/XBox-360 kind of gamepad). And we use a huge community maintained [database][1] of mappings, so your device would most likely be there. We use a slightly modified version of this database. See [#94](https://github.com/gabomdq/SDL_GameControllerDB/pull/94) and [#95](https://github.com/gabomdq/SDL_GameControllerDB/pull/95) for more details.

## Custom mappings?
Well I just realized while writing this post that we had forgotten about this :sweat_smile:.  But I have made a [PR](https://github.com/Kekun/gnome-games/pull/310) for it, so it should get merged soon. But as of now there is no GUI for it. Currently you can use Steam or the SDL test/controllermap tool to generate a custom mapping string as described [here](https://github.com/spurious/SDL-mirror/blob/release-2.0.4/src/joystick/SDL_gamecontrollerdb.h#L26-L30). Then you should paste in in a file in the user's config directory. As per this PR this file is `<config_dir>/gnome-games/gamecontrollerdb.txt` (`<config_dir>` is mostly `~/.config`).

## Multiplayer support
Multiplayer games are quite well supported. As of now there is no GUI for reassigning gamepads to other players, but the default behaviour is quite predictable. Just plugin the gamepads in the order of the players and all will be well.

The exact behaviour is this:

- the first player with no gamepad will be assigned the keyboard
- if there are *N* initially plugged-in gamepads, then they are assigned to the first *N* players and keyboard is assigned to player *N + 1*
- when a gamepad is plugged in, it is assigned to the first player with no gamepad (it may not be the last one), it can replace the keyboard
- when a gamepad is plugged out, its player shouldn't have any gamepad assigned but it shouldn't change the player to which other gamepads are assigned

## Next steps
The next steps involve adding a UI to remap the gamepads assigned too the players and then maybe a UI for remapping the controls if time permits.

Happy gaming!

[1]: https://github.com/gabomdq/SDL_GameControllerDB/blob/master/gamecontrollerdb.txt
[2]: https://github.com/spurious/SDL-mirror/blob/release-2.0.4/include/SDL_gamecontroller.h#L96-L108
