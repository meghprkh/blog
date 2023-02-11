---
title: GSoC Summary
date: 2016-08-22 17:11:26
tags: [ GSoC, GNOME ]
---

This post is meant to serve as a summary of work I did during GSoC. You may directly wish to skip to the [links section](#Links)

<!-- more -->

# The Goal
The goal to be achieved was to be able to play both single player and multiplayer emulated games using a gamepad in GNOME Games

# Where are we right now
- *Plug-n-play gamepad support:* We use the SDL mappings format and thus reuse a huge community DB of mappings to provide this.
- *Monitor plugin and unplugging of gamepads and react to it without user interaction:* The user should not be forced to take unnecessary steps to use the gamepads.
- *Multiplayer games/multiple gamepads support:* While there is no GUI for this the current way is intuitive and easy to use.  In fact, normally you won't have the need to use the GUI at all. [More about this here][1]
- *Separate codebase that can be reused (not a library yet):* The codebase leaves under a separate directory and has no dependencies on the main codebase. In fact, initially it was developed under a different namespace. Also in the process a library - [libgamepad][2] had been created. But I couldnt find the time to keep it up-to-date. Nevertheless it is functional currently also.

# What is left to be done
- Intuitive GUI to remap gamepads when multiple gamepads are plugged in. Mockups had been created and discussed ad the final design is ready (more about this in a future blog post)! You can check the current progress [here][3].

# Stretch Goals
I hope that I would be able to complete these during my winter vacation:

- Create a gamepad mapping creator UI (currently either Steam or SDL's controller map tool can be used for this)
- Update [libgamepad][2] to match the current codebase of Games.

# Challenges
- *Evdev and the Joystick API:* Initially WIP code had already been written by my mentor for the Joystick API and my project was to finish and polish that code. But later I came to know that the evdev API is the better API and that is what is used by all current libraries/programs (SDL/RetroArch). So I had to dig into the evdev API and start afresh.
- *VAPI for libevdev:* Vala requires us to create VAPIs for using C libraries (and you have to do it manually for non-Glib based ones). While creating the VAPI for libevdev I encountered a lot of challenges and got to know the Vala language much better.
- *Understanding the SDL codebase and the mappings format:* Now we needed a good mappings database. So we thought that it would be better if we could reuse some already existing database. So we decided to adopt the SDL mappings format, but I had some difficulty initially understanding their format (especially the GUID). Also I took inspiration from the SDL code and adopted it but I faced some difficulty understanding their C code.
- *Multiplayer support:* We needed an intuitive way which does not interfere with the user, i.e. we wanted a way to implement this so that the user will not have to do anything to play multiplayer games by default but if he wants he can configure the default assignment. After some brainstorming we decided on [this][1]

# Some technical details
- [Evdev][4] was used using [libevdev](https://www.freedesktop.org/software/libevdev/doc/latest/) to handle gamepads
- [Udev](https://en.wikipedia.org/wiki/Udev) was used using [GUdev](https://wiki.gnome.org/Projects/libgudev) for detecting plugging/unplugging of gamepads
- [SDL mappings format](https://github.com/spurious/SDL-mirror/blob/release-2.0.4/src/joystick/SDL_gamecontrollerdb.h#L26-L30) was used so that we have access to a large controller [mappings database](https://github.com/gabomdq/SDL_GameControllerDB). I even wrote a [python script][5] to validate the mappings and integrated with Travis so that the maintainer can easily merge the mappings.


# Links
- [Add Gamepad support - commits][0]
- [Multiplayer support][1]
- [libgamepad][2]
- [Multiple gamepads select UI WIP branch][3]
- [More details on the implemetation][4]
- [Python script to check SDL controller DB][5]


[0]: https://github.com/Kekun/gnome-games/commits/master?author=meghprkh
[1]: https://meghprkh.github.io/blog/2016/07/22/The-state-of-gamepad-support-in-Games/#Multiplayer-support
[2]: https://github.com/meghprkh/libgamepad
[3]: https://github.com/meghprkh/gnome-games/tree/dirty/feature/gamepad-select-ui-new
[4]: https://meghprkh.github.io/blog/2016/06/03/Handling-joysticks-and-gamepads-in-linux/
[5]: https://github.com/gabomdq/SDL_GameControllerDB/commits/master?author=meghprkh
