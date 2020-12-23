---
title: Handling joysticks and gamepads in linux
date: 2016-06-03 22:28:52
tags: [ GNOME, GSoC, Linux, Joystick, Gamepad ]
categories: Opensource
---
In this post I would share some of the things I came across when dealing with
the handling of joysticks and gamepads in Linux. One of the goals I wanted to
achieve was to make our controller mappings compatible with the SDL ones so that
we can reuse the community maintained controller mapping database that they have.

<!--more-->

The full code can be found [here](https://gist.github.com/meghprkh/9cdce0cd4e0f41ce93413b250a207a55).

The first thing that I want to clarify is that Linux provides *two* APIs for
dealing with joysticks. One is the legacy *joystick* API and the other is the
modern *evdev* API. The evdev-based API provides more detailed information about
the buttons and axes available and SDL2 only supports the evdev API so we
decided to go with the evdev API.

Quoting Arch Wiki:

> `/dev/input/jsX` maps to the 'Joystick' API interface and `/dev/input/event*`
> maps to the 'evdev' ones (this also includes other input devices such as mice
> and keyboards). Symbolic links to those devices are also available in
> `/dev/input/by-id/` and `/dev/input/by-path/` where the legacy 'Joystick' API
> has names ending with -joystick while the 'evdev' have names ending with
> `-event-joystick`.   

For using the evdev API, I decided to use the libevdev library instead of using traditional `ioctl` calls as this library provided simpler higher-level access to the evdev API.

Moving on to our main goal: we want to reuse the SDL mappings. The SDL mappings look something like these:
```
"guid,name,mappings"
"030000006d0400001dc2000014400000,Logitech F310 Gamepad (XInput),a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,"
```

Quoting SDL documentation:
> The mapping format for joystick is:
>     bX - a joystick button, index X
>     hX.Y - hat X with value Y
>     aX - axis X of the joystick
> Buttons can be used as a controller axis and vice versa.

In this post we will assume that we will handle the parsing of this mapping and only need to get the indexes correctly (like `b0`, `a2`, etc.)

## Generating GUID

So the first problem was to decipher how the GUID was generated. The GUID is an 128-bit code that is time and device independent. Its constructed using the bustype, vendor, product and version of the device. It is generated using the following code:

```c
void get_guid(struct libevdev * dev, guint16 * guid) {
    guid[0] = GINT16_TO_LE(libevdev_get_id_bustype(dev));
    guid[1] = 0;
    guid[2] = GINT16_TO_LE(libevdev_get_id_vendor(dev));
    guid[3] = 0;
    guid[4] = GINT16_TO_LE(libevdev_get_id_product(dev));
    guid[5] = 0;
    guid[6] = GINT16_TO_LE(libevdev_get_id_version(dev));
    guid[7] = 0;
}
```

As we want it to be device independent, we use the `GINT16_TO_LE` helper from glib to convert a 16 bit number to little endian.

But to convert this to string we convert it to its hexadecimal equivalent using the following simple code:

```c
void guid_to_string(guint16 * guid, char * guidstr) {
    static const char k_rgchHexToASCII[] = "0123456789abcdef";
    int i;
    for (i = 0; i < 8; i++) {
        unsigned char c = guid[i];

        *guidstr++ = k_rgchHexToASCII[c >> 4];
        *guidstr++ = k_rgchHexToASCII[c & 0x0F];

        c = guid[i] >> 8;
        *guidstr++ = k_rgchHexToASCII[c >> 4];
        *guidstr++ = k_rgchHexToASCII[c & 0x0F];
    }
    *guidstr = '\0';
}
```

## Feature Detection and mapping to the SDL indexes

Now coming to the feature detection part. We use the helper `libevdev_has_event_code (dev, type, code)` to detect if the device has a button/axis/hat. This way we loop over the possible values of the code for each type (`EV_KEY` for button, `EV_ABS` for axes and hat) and map it to an increasing number. That is the first valid axis code we found is `axis0` or `a0`, the second valid axis is `a1` and so on. It is the same for buttons.

For example, following is part of the code for buttons:
```c
int nbuttons = 0;
guint8 key_map[KEY_MAX];
for (i = BTN_JOYSTICK; i < KEY_MAX; ++i) {
    if (libevdev_has_event_code(dev, EV_KEY, i)) {
        printf("%d - Joystick has button: 0x%x - %s\n", nbuttons, i,
                libevdev_event_code_get_name(EV_KEY, i));
        key_map[i - BTN_MISC] = nbuttons;
        ++nbuttons;
    }
}
```

And while polling we find the button number through this `key_map`:
```c
printf("Button %d\n", key_map[ev.code - BTN_MISC]);
```

We do similar stuff for axes and hats even though the way we map changes. The hats mapping like `h0.4` can be done using a simple map from code and value. But SDL returns output as a 8-way dpad giving one of the eight values (like up, leftup, etc.) while evdev gives hat as two axes and reports two events: left and up on pressing the dpad/hat in the leftup direction.

## Conclusion

For polling events we use the `libevdev_next_event` function. The full **libevdev documentation** can be found [here](https://www.freedesktop.org/software/libevdev/doc/latest/)

The **full code** can be found [here](https://gist.github.com/meghprkh/9cdce0cd4e0f41ce93413b250a207a55). While this code uses glib, it only uses simple helper functions from glib which can be easily reimplemented. The only complex glib functions used are to detect the event-joystick device from the `/dev/input/by-path` folder. This code also doesnot have several fallbacks that the SDL code has.

My future work will involve the integration of this 'playground' code into the main GNOME Games code and also parsing the mapping. Other things that need to be done is to handle hats properly, handle fallbacks and see if we want to detect joystick devices by polling only or use udev.
