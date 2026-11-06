#include <glib.h>
#include <string.h>

#include "check.h"
#include "logout_policy.h"
#include "session_vars.h"

static const char *only_display(void *self, const char *name)
{
    (void) self;
    if (strcmp(name, "WAYLAND_DISPLAY") == 0) {
        return "wayland-1";
    }
    if (strcmp(name, "DISPLAY") == 0) {
        return "";
    }
    return NULL;
}

static const char *everything(void *self, const char *name)
{
    (void) self;
    (void) name;
    return "set";
}

static char **argv_for(const char *(*get)(void *self, const char *name))
{
    session_lookup lookup = { .self = NULL, .get = get };

    return session_vars_activation_argv("tool", &lookup);
}

static void test_argv_starts_with_tool_and_systemd(void)
{
    char **argv = argv_for(everything);

    CHECK(strcmp(argv[0], "tool") == 0);
    CHECK(strcmp(argv[1], "--systemd") == 0);
    g_strfreev(argv);
}

static void test_argv_skips_unset_and_empty_variables(void)
{
    char **argv = argv_for(only_display);

    CHECK_INT_EQ(3, g_strv_length(argv));
    CHECK(strcmp(argv[2], "WAYLAND_DISPLAY") == 0);
    g_strfreev(argv);
}

static void test_argv_exports_desktop_variables(void)
{
    char **argv = argv_for(everything);

    CHECK(g_strv_contains((const char *const *) argv, "XDG_CURRENT_DESKTOP"));
    CHECK(g_strv_contains((const char *const *) argv, "XDG_SESSION_DESKTOP"));
    CHECK(g_strv_contains((const char *const *) argv, "DESKTOP_SESSION"));
    CHECK(g_strv_contains((const char *const *) argv, "XDG_MENU_PREFIX"));
    CHECK(g_strv_contains((const char *const *) argv, "DISPLAY"));
    CHECK(g_strv_contains((const char *const *) argv, "QT_QPA_PLATFORM"));
    g_strfreev(argv);
}

static const char *default_of(const char *name)
{
    for (size_t i = 0; session_vars_defaults[i].name; i++) {
        if (strcmp(session_vars_defaults[i].name, name) == 0) {
            return session_vars_defaults[i].value;
        }
    }
    return NULL;
}

static void test_defaults_describe_an_xfce_session(void)
{
    CHECK(strcmp(default_of("XDG_CURRENT_DESKTOP"), "XFCE") == 0);
    CHECK(strcmp(default_of("XDG_SESSION_DESKTOP"), "xfce") == 0);
    CHECK(strcmp(default_of("DESKTOP_SESSION"), "xfce") == 0);
    CHECK(strcmp(default_of("XDG_MENU_PREFIX"), "xfce-") == 0);
    CHECK(strcmp(default_of("QT_QPA_PLATFORM"), "wayland;xcb") == 0);
    CHECK(default_of("WAYLAND_DISPLAY") == NULL);
}

static void test_logout_falls_back_when_manager_missing(void)
{
    CHECK(logout_manager_unavailable("org.freedesktop.DBus.Error.ServiceUnknown"));
    CHECK(logout_manager_unavailable("org.freedesktop.DBus.Error.NameHasNoOwner"));
    CHECK(logout_manager_unavailable("org.freedesktop.DBus.Error.UnknownMethod"));
}

static void test_logout_keeps_other_errors(void)
{
    CHECK(!logout_manager_unavailable("org.xfce.SessionManager.Error.Failed"));
    CHECK(!logout_manager_unavailable("org.freedesktop.DBus.Error.NoReply"));
    CHECK(!logout_manager_unavailable(NULL));
}

int main(void)
{
    static const check_case cases[] = {
        { "argv_starts_with_tool_and_systemd", test_argv_starts_with_tool_and_systemd },
        { "argv_skips_unset_and_empty_variables", test_argv_skips_unset_and_empty_variables },
        { "argv_exports_desktop_variables", test_argv_exports_desktop_variables },
        { "defaults_describe_an_xfce_session", test_defaults_describe_an_xfce_session },
        { "logout_falls_back_when_manager_missing", test_logout_falls_back_when_manager_missing },
        { "logout_keeps_other_errors", test_logout_keeps_other_errors },
    };

    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}
