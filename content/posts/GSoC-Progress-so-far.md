---
title: GSoC - Progress so far
date: 2016-06-26 18:58:22
tags: [ GNOME, GSoC, Gamepad, Library, Games, GNOME Games ]
categories: Opensource
---
My project is about adding gamepad support to GNOME Games. This means that soon you would be able to play your favorite retro games using a gamepad!

Currently you can plugin a gamepad and it will just work! The app will automatically detect it and allow you to play your favorite game using the gamepad. Support for playing multiplayer games is also there. The Games branch which supports gamepads can be found [here](https://github.com/meghprkh/gnome-games/tree/dirty/feature/gamepad-incremental).

<!-- more -->

## How its being done
To integrate gamepads a library was made. While currently this library is being developed as part of the GNOME Games codebase. Its developed under a different namespace. You can find the library code [here](https://github.com/meghprkh/libgamepad). Please note that the API of this library is *unstable* and will keep changing.

This library is a small GLib-based library that is written in Vala and aims to be easy to use. While the long-term goal is to be cross-platform, currently it supports Linux only.

The gamepad mapping format used is compatible with the [SDL mappings](https://github.com/gabomdq/SDL_GameControllerDB). This means that you would be able to use many gamepads without requiring to configure them.

## What's there in the future?
After some more cleanup, this code will hopefully be merged into the Games codebase. After that I will focus on adding UI for features like easy-assigning of gamepads to ports and remapping the gamepad.

I will keep posting updates on this blog. Stay tuned!

## Related posts
- {% post_link GSoC-2016-Introduction %}
- {% post_link Handling-joysticks-and-gamepads-in-linux %}
