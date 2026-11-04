# xfwm-wayland

`xfwm-wayland` is a Wayland compositor for Xfce that behaves like
[xfwm4-gl](../xfwm4-gl): same themes, same settings, same shortcuts, same
window management. It is built on wlroots 0.20 and runs X11 applications
through XWayland.

## Build

Needs wlroots 0.20, wayland, xkbcommon, libinput, pixman, cairo, pango,
gdk-pixbuf, glib, json-glib, librsvg, xfconf and GTK 3 (for the helper program).

    meson setup build
    ninja -C build
    meson test -C build
    sudo ninja -C build install

If the xfconf headers are not installed system-wide, point `PKG_CONFIG_PATH`
at them first.

## Usage

Display managers list a "Xfce Session (xfwm-wayland)" entry. Pick it at login.

From a text console, run it alone. It starts `xfce4-session`:

    xfwm-wayland

Other ways to start it:

    startxfce4 --wayland xfwm-wayland
    xfwm-wayland --startup xfce4-terminal
    xfwm-wayland --no-session

`--startup COMMAND` runs `COMMAND` instead of `xfce4-session`. xfwm-wayland
quits when the session program ends, whatever its exit status. `--no-session`
runs no program.

Before the session starts, xfwm-wayland sets `WAYLAND_DISPLAY`, `DISPLAY`,
`XDG_SESSION_TYPE`, and `XDG_CURRENT_DESKTOP`, `XDG_SESSION_DESKTOP`,
`DESKTOP_SESSION`, `XDG_MENU_PREFIX` when they are not set. It then passes them
to D-Bus and systemd activation with `dbus-update-activation-environment`.
With `--no-session` it changes only its own environment. If no D-Bus session bus
is running, it starts a private one and stops it on exit.

The logout shortcut asks `xfce4-session` to log out over D-Bus. If the session
manager is not running, it runs `xfce4-session-logout`.

Settings are read from the same xfconf channels as xfwm4 (`xfwm4`,
`xfce4-keyboard-shortcuts`), so the Window Manager, Window Manager Tweaks and
Workspaces dialogs work as they do with xfwm4. Keyboard layout, key repeat,
pointer and cursor settings come from the `keyboard-layout`, `keyboards`,
`pointers` and `xsettings` channels, because on Wayland the compositor applies
them instead of xfsettingsd.

## What maps differently on Wayland

* **Use the GPU for compositing** picks the GPU renderer; off uses the
  software renderer. The change takes effect at the next login. Compositing
  itself cannot be turned off on Wayland; turning it off in the settings
  removes shadows, transparency, zoom and Alt+Tab previews.
* **Show focused fullscreen windows directly** lets a fullscreen window go
  straight to the screen (direct scanout).
* **vblank_mode**: `auto` syncs every frame, `off` allows tearing, `adaptive`
  uses variable refresh rate where the screen supports it. Fullscreen games
  that ask for tearing get it.
* **ten_bit_color** (Settings Editor, `xfwm4` channel): `true` draws
  the screen in 10 bits per color, falling back to 8 bits where the screen or
  driver refuses. Off by default.
* **hdr** (Settings Editor, `xfwm4` channel): `true` turns on HDR (HDR10) on
  every monitor that supports it, in 10-bit color; other monitors stay SDR.
  HDR needs the Vulkan renderer: xfwm-wayland switches to it in the running
  session when HDR is turned on, and back when it is turned off (the screen
  may blink once). If Vulkan cannot drive the screens it switches straight
  back and HDR stays off. Programs started before the switch keep the buffer
  formats of the old renderer, so a few (video players, games) may need a
  restart. Normal content is shown at a fixed 203 nits; HDR-aware programs
  (mpv, games) send real HDR through the color-management protocol. With
  Vulkan, `max_render_time=auto` acts as `off`.
* **hdr_brightness** (Window Manager Tweaks, under "Enable HDR"): brightness
  of the whole picture while HDR is on, from 50% to 200% (default 100%, normal
  content at 203 nits). It changes HDR videos and games too, because wlroots
  0.20 cannot change normal content alone. Night-light programs override it
  while they run.
* **max_render_time** (Settings Editor, `xfwm4` channel) lowers input lag by
  drawing the screen as late as possible before each refresh. `auto` (default)
  measures the drawing time (OpenGL renderer only, otherwise acts as `off`);
  a number of milliseconds keeps that much time for drawing; `off` draws right
  away. It does nothing while variable refresh rate or tearing is active.
  Too small a value makes frames miss the refresh and stutter.
* xfwm-wayland asks for realtime CPU priority at startup, so the screen keeps
  up when every core is busy. A tiny helper, `xfwm-wayland-realtime`, does it:
  it needs the `cap_sys_nice` capability (`setcap cap_sys_nice+ep
  /usr/libexec/xfwm-wayland-realtime`, done by the package), raises only the
  program that started it to the lowest realtime level, and exits. Without it,
  xfwm-wayland runs at normal priority. Programs it starts get normal priority.
* Drag and drop works as on X11: a left-button drag moves or copies directly;
  dragging with the right or middle button, or holding Alt, asks what to do.
* The cursor is drawn at exactly the size set in Mouse and Touchpad. Themes
  with scalable (SVG) cursors, in KDE's `cursors_scalable` format, stay sharp
  at any size; other themes have their nearest image scaled.
* A monitor seen for the first time gets a scale guessed from its size and
  resolution (1 to 3, in steps of 0.25). Monitors that report no real size,
  and TVs, get 1. The Display settings override it.
* Windows that draw their own title bar (most GTK 4 and some GTK 3
  applications) keep it; the xfwm4 theme frames the others.
* Programs that support Wayland window sessions (`xdg-session-management`)
  get their window size, position, maximized or fullscreen state and workspace
  back when they start again. This is kept in
  `$XDG_STATE_HOME/xfwm-wayland/sessions` (`~/.local/state/...` by default);
  sessions unused for 90 days are deleted.
* The panel's window buttons and window menu can shade, stick, keep above or
  below, move, resize and send windows to another workspace or monitor, and
  show the window's own icon. Highlighting a window draws the move outline
  around it.
* Find Cursor (Accessibility settings) draws red rings around the pointer
  when an Xfce program asks for it. Turning it off in the `accessibility`
  channel (`/FindCursor`) disables it.
* Mouse and Touchpad settings list the devices and their settings over Wayland.
  Deleting a property in the `pointers` channel puts the device back to its
  default. Touchscreens and tablets follow their assigned monitor, rotation and
  reflection; `/Properties/libinput_Calibration_Matrix` (6 or 9 numbers) sets
  the calibration. `Coordinate_Transformation_Matrix` written on X11 is ignored.
* Programs that support tablets get pen pressure, tilt and buttons; other
  programs see the pen as a mouse, with the side buttons as middle and right.
* VR headsets are not part of the desktop. VR programs (SteamVR, Monado) take
  them over with DRM leasing, except while the screen is locked.
* Translations live in `po/` (69 languages). The "Monitor Left/Right/Up/Down"
  translations of 34 of them come from [xfwl4](https://gitlab.xfce.org/xfce/xfwl4)
  and are under the GNU GPL version 3 or later.
* `XFWM4_PAINT_STATS=1` prints the number of frames painted per second every
  5 seconds.
