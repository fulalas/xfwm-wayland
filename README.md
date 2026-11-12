# xfwm-wayland

A Wayland compositor for Xfce built on `wlroots` that behaves like [xfwm4](https://gitlab.xfce.org/xfce/xfwm4):
same themes, settings, shortcuts and window management. X11 programs run
through `XWayland`.

It supports:

* `xfwm4` themes, the Window Manager, Tweaks and Workspaces settings and
  Xfce keyboard shortcuts
* HDR10, switched on and off without logging out, with a brightness slider
* 10-bit color, variable refresh rate, and tearing (frames shown without
  waiting for the screen refresh, for lower input lag) in fullscreen
  programs that ask for it
* Xfce display profiles, fractional scaling and multiple monitors
* Alt+Tab with window previews, the window menu, snapping, tiling, zoom,
  shadows and opacity
* screenshots and screen sharing (through `xdg-desktop-portal-wlr`),
  touchscreens, drawing tablets and VR headsets
* restoring window size and position for programs that support it
* window rules, set from scripts through `xfconf`
* 69 languages

## Why this exists

[Xfwl4](https://gitlab.xfce.org/xfce/xfwl4/), the official Xfce Wayland compositor, only started
development in January 2026 and is still not ready for daily use. Also,
some differences seem intrinsic to the project, so they're unlikely to
change.

Here's a quick comparison:

|                                      |  xfwl4             |  xfwm-wayland            |
|--------------------------------------|--------------------|--------------------------|
| Language                             | Rust, on `Smithay` | C, on `wlroots`          |
| Dependencies built                   | 286                | 23                       |
| Build time (release)                 | 138 s              | 5 s                      |
| Program size                         | 26.4 MB            | 0.7 MB                   |
| Memory consumption                   | 158.5 MB           | 33.4 MB                  |
| Average power                        | 9.6 W              | 8.9 W                    |
| Power with a fullscreen window       | 8.6 W              | 6.7 W                    |
| Compositor CPU time                  | 10.9 s             | 7.2 s                    |
| Frame to screen delay                | 19.5 ms            | 16.8 ms                  |
| Unit tests                           | 49                 | 795                      |
| HDR                                  | no                 | yes                      |
| Variable refresh rate                | no                 | yes                      |
| Tearing (optional)                   | no                 | yes                      |
| Busy cursor while apps start         | no                 | yes                      |
| xfwm4 compositing settings           | some ignored       | all                      |
| Renderers                            | OpenGL             | OpenGL, Vulkan, CPU      |
| Sharp X11 programs when scaling      | yes, uneven sizes  | yes                      |

Power, CPU time and delay come from [wmbench](https://github.com/fulalas/wmbench) on an AMD Ryzen 7 7840HS at
3840x2160, 60 Hz. Memory is the compositor's total resident memory at scale 1,
right after login.

## Build

Needs `wlroots` 0.20+, wayland, wayland-protocols, xkbcommon, libinput,
libdrm, EGL, xcb (with ewmh, icccm and shape), pixman, cairo, pango,
gdk-pixbuf, glib, json-glib, librsvg, `xfconf` and GTK 3.

    meson setup build
    ninja -C build
    sudo ninja -C build install

## Usage

Pick "Xfce Session (xfwm-wayland)" at login, or start Xfce manually:

    startxfce4 --wayland xfwm-wayland

Login managers remember the last session picked. For example, to make it the
default in LightDM, set it in `/etc/lightdm/lightdm.conf`:

    [Seat:*]
    user-session=xfwm-wayland

Settings are read from the same `xfconf` channels as `xfwm4`. Keyboard, mouse
and cursor settings are applied by the compositor instead of `xfsettingsd`.

Although it works with `xfwm4`, [xfwm4-gl](https://github.com/fulalas/xfwm4-gl) is highly recommended because:

* its settings dialogs open on Wayland; the original ones check for an X11
  window manager and refuse to start
* it hides the options that do nothing on Wayland and renames the ones that
  work differently
* its bundled themes are SVG, so window borders stay sharp at any scale
* its Tweaks dialog has the HDR and HDR brightness options:

<img width="926" height="785" alt="Tweaks dialog" src="https://github.com/user-attachments/assets/d054024e-a77c-4581-8961-37164a02b04f" />

## Differences from X11

Screen

* **Use the GPU for compositing** applies at the next login. Compositing can't
  be fully off: turning it off only removes shadows, transparency, zoom and
  Alt+Tab previews.
* `vblank_mode`: `auto`, `adaptive` (variable refresh rate) or `off` (no wait
  for the refresh: lowest input lag, but may tear). Fullscreen programs that
  ask for tearing get it even with `auto`. `glx` and `xpresent` act as `auto`.
* `max_render_time` (new): draws as late as possible before each refresh to
  lower input lag. `auto` (default) measures it, a number sets the
  milliseconds, `off` draws right away.
* `ten_bit_color` (new): 10-bit color, falling back to 8-bit.
* **Enable HDR** (new): HDR10 on monitors that support it. Switches to Vulkan
  (the screen may blink), or stays off if Vulkan can't drive the screens.
  Running programs may need a restart. Normal content is shown at 203 nits;
  **HDR brightness** (50% to 200%) scales everything, HDR videos included.
  Night-light programs override it while running.
* New monitors get a scale guessed from their size (1 to 3); TVs get 1.

Input

* The cursor is drawn at the exact size set. SVG cursor themes stay smooth;
  others scale their nearest size without smoothing, like X11.
* Touchscreen and tablet calibration uses `libinput_Calibration_Matrix`
  instead of `Coordinate_Transformation_Matrix`.
* Programs get global shortcuts through the GlobalShortcuts portal. Only
  special keys (function, media, etc.) or keys with Ctrl, Alt or Super can be
  global. Xfce shortcuts win over them.
* A shortcut on a modifier key alone (e.g. `Super_L` to open the
  applications menu) runs when the key is released, only if no other key or
  mouse button was pressed meanwhile, like `xcape` on X11.
* Displays turn off when idle. The timeout is taken from
  `~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-power-manager.xml`
  (if the file is not found, 10 minutes on AC or 5 on battery).
* Programs keep the screen on through Wayland's idle inhibit or
  `org.freedesktop.ScreenSaver` (xfwm-wayland provides it when no screensaver
  does). X11's own screensaver calls don't work through `XWayland`.
* X11 programs with global hotkeys (e.g. Audacious) still get those keys when
  a Wayland window has focus. Typed text isn't shared.

Windows

* Sessions use the Wayland protocol instead of the X11 session manager:
  programs that support it get their size, position, state and workspace
  back. They're kept 90 days in `~/.local/state/xfwm-wayland/sessions`.
* SVG theme images scale sharply, like `xfwm4-gl`, and win over a bitmap of
  the same name. Other themes are drawn pixel for pixel, like X11.

Other

* `xfwm-wayland-realtime` gives the compositor realtime priority so the screen
  keeps up under load. It needs `cap_sys_nice`: the package sets it; from
  source, run `sudo setcap cap_sys_nice+ep /usr/local/libexec/xfwm-wayland-realtime`.
* `--compositor on|off|auto` and `--vblank` override the settings until they
  change or the next login, like `xfwm4`. `--replace` is ignored.

## Scaling

Wayland programs stay sharp when scaled. There's no font DPI setting, so
everything is scaled with the Scale value in the Display settings:

<img width="1083" height="740" alt="Display settings" src="https://github.com/user-attachments/assets/97011ed2-e015-44c0-b75c-b181512a3a59" />

Also, icons and the mouse cursor may have different sizes compared to X11 with a
similar font DPI. That's expected.

X11 programs draw at full resolution and scale themselves, so they also stay
sharp. `Xft.dpi` and the X11 DPI setting become 96 × the largest output scale,
which GTK and Qt programs follow. On outputs with a lower scale, X11 programs
are shrunk. Running programs may need a restart after the scale changes.

## Issues and possible solutions

* X11 programs that ignore the DPI (Java, old toolkits) look small when
  scaled. Turning off `xwayland_zero_scaling` makes the compositor scale
  them instead, but blurry:
  ```sh
  xfconf-query -c xfwm4 -p /general/xwayland_zero_scaling -n -t bool -s false
  ```
* Many programs that support Wayland still use `XWayland` by default, which
  means more CPU and input lag, possible issues with multiple displays and no
  HDR. Set these to make them use Wayland when they can:
  ```sh
  QT_QPA_PLATFORM="wayland;xcb"
  GDK_BACKEND=wayland,x11
  ELECTRON_OZONE_PLATFORM_HINT=auto
  SDL_VIDEODRIVER=wayland,x11
  ```
* Old Chromium-based browsers and Electron programs may need the parameter
  `--ozone-platform-hint=auto`.
* Window positions come back only for Wayland programs that support
  `xdg-session-management`; X11 programs never get them back.
* Ctrl+Shift drag doesn't create symlinks (Wayland limitation).
* No keyboard layout plugin in the panel (Xfce plugin is currently X11 only).
* Window screenshots (Alt+Print) and correct area offsets need the 2 patches [here](https://github.com/porteux/porteux/tree/3.0/003-xfce/xfce/xfce4-screenshooter).
* Multimedia keys need the patch [here](https://github.com/porteux/porteux/tree/3.0/003-xfce/xfce/xfce4-pulseaudio-plugin).
* Power manager shortcuts need the patch [here](https://github.com/porteux/porteux/tree/3.0/003-xfce/xfce/xfce4-power-manager).

## Window rules

Window rules are kept in the `xfwm4` channel under `/window-rules/<n>/`,
where `<n>` is a number. They apply when a window opens, and again to open
windows whenever a rule changes, so a script can move a window by editing its
rule. When several rules match, the higher number wins for each property.

| Property       | Type   | Value                                                       |
|----------------|--------|-------------------------------------------------------------|
| `title`        | string | window title                                                |
| `app_id`       | string | Wayland app id, or X11 class (second value of `xprop WM_CLASS`) |
| `match`        | string | `exact` (default) or `glob` (`*`, `?`, `[...]`)             |
| `position`     | string | `top-left`, `top`, `top-right`, `left`, `center`, `right`, `bottom-left`, `bottom`, `bottom-right` |
| `margin`       | int    | gap to the screen edges, in pixels at scale 1               |
| `output`       | string | output name, e.g. `DP-1` (see `wlr-randr`)                  |
| `workspace`    | int    | workspace number, from 1                                    |
| `above`        | bool   | keep above other windows                                    |
| `skip_taskbar` | bool   | hide from the taskbar                                       |
| `no_focus`     | bool   | never take the keyboard focus                               |

A rule needs a `title` or an `app_id`. `position` places the window in the
work area (screen minus panels) of its output and keeps its size. Maximized
and fullscreen windows keep their place. Rules don't run again when a window
changes its title.

Add a rule, then move the window:

    xfconf-query -c xfwm4 -p /window-rules/0/app_id -n -t string -s org.xfce.mousepad
    xfconf-query -c xfwm4 -p /window-rules/0/position -n -t string -s bottom-right
    xfconf-query -c xfwm4 -p /window-rules/0/margin -n -t int -s 10
    xfconf-query -c xfwm4 -p /window-rules/0/position -s top-left

Remove it with `xfconf-query -c xfwm4 -p /window-rules/0 -r -R`. Open windows
keep what the rule gave them (position, above, taskbar, focus).

## License

Same as `xfwm4`: GNU General Public License, version 2 or later. See
[COPYING](COPYING) for the full text.
