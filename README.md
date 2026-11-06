# xfwm-wayland

A Wayland compositor for Xfce built on `wlroots` that behaves like [xfwm4](https://gitlab.xfce.org/xfce/xfwm4):
same themes, settings, shortcuts and window management. X11 programs run
through `XWayland`.

It supports:

* `xfwm4` themes, the Window Manager, Tweaks and Workspaces settings and
  Xfce keyboard shortcuts
* Alt+Tab with window previews, the window menu, snapping, tiling, zoom,
  shadows and opacity
* HDR10, switched on and off without logging out, with a brightness slider
* 10-bit color, variable refresh rate, and tearing (frames shown without
  waiting for the screen refresh, for lower input lag) in fullscreen
  applications that ask for it
* Xfce display profiles, fractional scaling and multiple monitors
* screenshots and screen sharing (through `xdg-desktop-portal-wlr`),
  touchscreens, drawing tablets and VR headsets
* restoring window size and position for programs that support it
* 69 languages

## Why this exists

[Xfwl4](https://gitlab.xfce.org/xfce/xfwl4/), the official Xfce Wayland compositor, only started
development in January 2026 and is still not ready for daily use. Also,
some differences seem to be intrinsic to the project so it doesn't seem
it's going to change.

Here's a quick comparison:

|                                      |  xfwl4             |  xfwm-wayland            |
|--------------------------------------|--------------------|--------------------------|
| Written in                           | Rust, on `Smithay` | C, on `wlroots`          |
| Dependencies built                   | 286                | 23                       |
| Build time (release)                 | 138 s              | 4 s                      |
| Program size                         | 26.4 MB            | 0.7 MB                   |
| Average power                        | 9.6 W              | 8.6 W                    |
| Power with a fullscreen window       | 8.6 W              | 6.4 W                    |
| Compositor CPU time                  | 10.9 s             | 7.0 s                    |
| Frame to screen delay                | 19.5 ms            | 16.7 ms                  |
| Unit tests                           | 49                 | 733                      |
| HDR                                  | no                 | yes                      |
| Variable refresh rate                | no                 | yes                      |
| Tearing (optional)                   | no                 | yes                      |
| Busy cursor while apps start         | no                 | yes                      |
| xfwm4 compositing settings           | some ignored       | all                      |
| Renderers                            | OpenGL             | OpenGL, Vulkan, software |
| Sharp X11 programs when scaling      | yes                | no                       |

Power, CPU time and delay come from [wmbench](https://github.com/fulalas/wmbench) on an AMD Ryzen 7 7840HS at
3840x2160, 60 Hz.

## Build

Needs `wlroots` 0.20, wayland, xkbcommon, libinput, pixman, cairo, pango,
gdk-pixbuf, glib, json-glib, librsvg, `xfconf` and GTK 3.

    meson setup build
    ninja -C build
    sudo ninja -C build install

## Usage

Pick "Xfce Session (xfwm-wayland)" at login, or start Xfce manually:

    startxfce4 --wayland xfwm-wayland

Login managers remember the last session picked. To make it the default
using, for example, LightDM, set it in `/etc/lightdm/lightdm.conf`:

    [Seat:*]
    user-session=xfwm-wayland

Settings are read from the same `xfconf` channels as `xfwm4`. Keyboard, mouse
and cursor settings are applied by the compositor instead of `xfsettingsd`.

Although it works with `xfwm4`, [xfwm4-gl](https://github.com/fulalas/xfwm4-gl) is highly recommended because:

* its settings dialogs open on Wayland; the original ones check for an X11
  window manager and refuse to start
* its Tweaks dialog has the HDR and HDR brightness options
* it hides the options that do nothing on Wayland and renames the ones that
  work differently
* its bundled themes are SVG, so window borders stay sharp at any scale

## Differences from X11

Screen

* **Use the GPU for compositing**: takes effect at the next login. Compositing
  itself can't be turned off; turning it off removes shadows, transparency,
  zoom and Alt+Tab previews.
* `vblank_mode`: `auto`, `adaptive` for variable refresh rate, or `off` to
  show every frame right away (lowest input lag, but moving images can tear).
  Fullscreen applications that ask for tearing get it even with `auto`. `glx`
  and `xpresent` act as `auto`.
* `max_render_time` (new): draws as late as possible before each refresh to
  lower input lag. `auto` (default) measures the drawing time (OpenGL renderer
  only), a number keeps that many milliseconds, `off` draws right away.
* `ten_bit_color` (new): 10 bits per color, falling back to 8 bits.
* **Enable HDR** (new): HDR10 on monitors that support it. It switches to the
  Vulkan renderer (the screen may blink), or stays off if Vulkan can't drive
  the screens. Programs started before the switch may need a restart. Normal
  content is shown at 203 nits; **HDR brightness** (50% to 200%) scales the
  whole picture, HDR videos included. Night-light programs override it while
  they run.
* A monitor seen for the first time gets a scale guessed from its size (1 to
  3); TVs get 1.

Input

* The cursor is drawn at exactly the size set. Cursor themes with scalable
  (SVG) cursors stay smooth at any size; other themes have their nearest size
  scaled without smoothing, like X11.
* Touchscreen and tablet calibration uses `libinput_Calibration_Matrix`
  instead of X11's `Coordinate_Transformation_Matrix`.
* Programs register global shortcuts through the GlobalShortcuts portal
  (`xdg-desktop-portal`), since Wayland has no global key grabs. Only function,
  media and other special keys, or keys pressed with Ctrl, Alt or Super, can be
  global. Your own Xfce shortcuts win over them.
* When xfce4-power-manager is not running, displays still switch off after the
  display sleep or switch-off time from its settings (10 minutes by default),
  like X11's built-in timer. Battery times apply when UPower reports battery.
* Video players and browsers keep the screen on through Wayland's idle inhibit
  or `org.freedesktop.ScreenSaver`, which xfwm-wayland provides when no
  screensaver program does. X11 programs can't use X11's own screensaver calls,
  because Xwayland doesn't support them.
* X11 programs that grab keys globally (for example Audacious's hotkeys) still
  get those same keys when a Wayland window has focus. Typed text is not
  shared.

Windows

* Window sessions use the Wayland protocol instead of the X11 session manager:
  programs that support it get their size, position, state and workspace back.
  They are kept in `~/.local/state/xfwm-wayland/sessions` for 90 days.
* Themes with SVG images scale with the screen and stay sharp, like
  `xfwm4-gl`. An SVG image wins over a bitmap of the same name. Other themes
  are drawn pixel for pixel, like X11.
* X11 programs look blurry on scaled screens: `wlroots` 0.20 can't scale
  `XWayland` sharply (see
  [wlroots !2064](https://gitlab.freedesktop.org/wlroots/wlroots/-/merge_requests/2064)).

Other

* `xfwm-wayland-realtime` gives the compositor realtime priority so the screen
  keeps up under load. It needs `cap_sys_nice`: the package sets it; after
  installing from source, run
  `sudo setcap cap_sys_nice+ep /usr/local/libexec/xfwm-wayland-realtime`.
* `--compositor on|off|auto` and `--vblank` work like in `xfwm4`: they override
  the settings until those are changed or the next login. `--replace` is
  accepted and ignored.

## License

Same as `xfwm4`: GNU General Public License, version 2 or later. See
[COPYING](COPYING) for the full text.
