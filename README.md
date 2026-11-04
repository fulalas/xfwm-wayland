# xfwm-wayland

`xfwm-wayland` is a Wayland compositor for Xfce that behaves like
[xfwm4-gl](../xfwm4-gl): same themes, same settings, same shortcuts, same
window management. It is built on wlroots 0.20 and runs X11 applications
through XWayland.

## Build

Needs wlroots 0.20, wayland, xkbcommon, libinput, pixman, cairo, pango,
gdk-pixbuf, glib, xfconf and GTK 3 (for the helper program).

    meson setup build
    ninja -C build
    meson test -C build
    sudo ninja -C build install

If the xfconf headers are not installed system-wide, point `PKG_CONFIG_PATH`
at them first.

## Usage

Start an Xfce session on Wayland with it:

    startxfce4 --wayland xfwm-wayland --startup xfce4-session

Or start it alone from a text console, with an optional first program:

    xfwm-wayland --startup xfce4-terminal

Settings are read from the same xfconf channels as xfwm4 (`xfwm4`,
`xfce4-keyboard-shortcuts`), so the Window Manager, Window Manager Tweaks and
Workspaces dialogs work as they do with xfwm4. Keyboard layout, key repeat,
pointer and cursor settings come from the `keyboard-layout`, `keyboards`,
`pointers` and `xsettings` channels, because on Wayland the compositor applies
them instead of xfsettingsd.

## What maps differently on Wayland

* **Use OpenGL for compositing** picks the GPU renderer; off uses the software
  renderer. Compositing itself cannot be turned off on Wayland; turning it off
  in the settings removes shadows, transparency, zoom and Alt+Tab previews.
* **Suspend compositing for focused fullscreen windows** lets a fullscreen
  window go straight to the screen (direct scanout).
* **vblank_mode**: `auto` syncs every frame, `off` allows tearing, `adaptive`
  uses variable refresh rate where the screen supports it. Fullscreen games
  that ask for tearing get it.
* Windows that draw their own title bar (most GTK 4 and some GTK 3
  applications) keep it; the xfwm4 theme frames the others.
* `XFWM4_PAINT_STATS=1` prints the number of frames painted per second every
  5 seconds.
