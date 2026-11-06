xfce_glib_dep = dependency('glib-2.0')
xfce_gio_dep = dependency('gio-2.0')
xfce_gio_unix_dep = dependency('gio-unix-2.0')
xfce_xfconf_dep = dependency('libxfconf-0')
xfce_xkbcommon_dep = dependency('xkbcommon')
xfce_cairo_dep = dependency('cairo')
xfce_pixbuf_dep = dependency('gdk-pixbuf-2.0')

xfce_inc = [include_directories('include'), core_inc, compositor_api_inc]

xfce_helper_install_path = get_option('prefix') / get_option('libexecdir') / 'xfwm-wayland-helper'

xfce_logic = static_library(
  'xfce_logic',
  files(
    'src/accelerator.c',
    'src/app_shortcuts.c',
    'src/accessibility_resolve.c',
    'src/changes.c',
    'src/config_table.c',
    'src/config_values.c',
    'src/display_choice.c',
    'src/display_off.c',
    'src/display_plan.c',
    'src/display_read.c',
    'src/display_table.c',
    'src/display_transform.c',
    'src/display_write.c',
    'src/helper_protocol.c',
    'src/input_resolve.c',
    'src/input_text.c',
    'src/keymap_store.c',
    'src/logout_policy.c',
    'src/pointer_choices.c',
    'src/pointer_resolve.c',
    'src/session_vars.c',
    'src/setting_source.c',
    'src/shortcut_defaults.c',
    'src/shortcut_property.c',
    'src/style_state.c',
    '../../helper/protocol_text.c',
  ),
  include_directories: [xfce_inc, include_directories('../../helper')],
  dependencies: [wm_core_dep, xfce_glib_dep, xfce_xkbcommon_dep, xfce_cairo_dep, display_hash_dep],
)

xfce_lib = static_library(
  'xfce',
  files(
    'src/accessibility.c',
    'src/channels.c',
    'src/dispatcher.c',
    'src/display_store.c',
    'src/environment.c',
    'src/gtk_bridge.c',
    'src/helper_client.c',
    'src/helper_path.c',
    'src/helper_session.c',
    'src/icon_cache.c',
    'src/icon_lookups.c',
    'src/icon_surface.c',
    'src/input_prefs.c',
    'src/logout.c',
    'src/power_source.c',
    'src/screensaver_service.c',
    'src/session_export.c',
    'src/session_launch.c',
    'src/settings.c',
    'src/shortcuts.c',
    'src/shortcuts_portal.c',
    'src/spawn.c',
    'src/terminate.c',
    'src/x11_setting_values.c',
    'src/x11_settings.c',
    'src/xfconf_source.c',
  ),
  include_directories: xfce_inc,
  c_args: ['-DXFCE_HELPER_INSTALL_PATH="@0@"'.format(xfce_helper_install_path)],
  dependencies: [
    wm_core_dep,
    xfce_glib_dep,
    xfce_gio_dep,
    xfce_gio_unix_dep,
    xfce_xfconf_dep,
    xfce_xkbcommon_dep,
    xfce_cairo_dep,
    xfce_pixbuf_dep,
  ],
)

xfce_dep = declare_dependency(
  link_with: [xfce_lib, xfce_logic],
  include_directories: xfce_inc,
  dependencies: [
    wm_core_dep,
    xfce_glib_dep,
    xfce_gio_dep,
    xfce_gio_unix_dep,
    xfce_xfconf_dep,
    xfce_xkbcommon_dep,
    xfce_cairo_dep,
    xfce_pixbuf_dep,
  ],
)

xfce_tests = ['accelerator', 'app_shortcuts', 'accessibility', 'config', 'display_off', 'displays', 'input', 'pointer_prefs', 'protocol', 'session', 'shortcuts']

foreach name : xfce_tests
  test(
    'xfce_' + name,
    executable(
      'test_xfce_' + name,
      'tests/test_' + name + '.c',
      include_directories: [xfce_inc, include_directories('src')],
      link_with: xfce_logic,
      dependencies: [check_dep, xfce_glib_dep, xfce_xkbcommon_dep, xfce_cairo_dep],
    ),
  )
endforeach

test(
  'xfce_wire',
  executable(
    'test_xfce_wire',
    'tests/test_wire.c',
    include_directories: [xfce_inc, include_directories('src', '../../helper')],
    link_with: xfce_logic,
    dependencies: [check_dep, xfce_glib_dep, xfce_cairo_dep],
  ),
)

test(
  'xfce_x11_settings',
  executable(
    'test_xfce_x11_settings',
    'tests/test_x11_settings.c',
    'src/x11_setting_values.c',
    include_directories: [xfce_inc, include_directories('src')],
    dependencies: [check_dep, xfce_glib_dep, dependency('gobject-2.0')],
  ),
)
