#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"

static const char usage[] =
    "Usage: xfwm-wayland [-s|--startup COMMAND | -n|--no-session] [-c|--compositor on|off|auto]\n"
    "                    [-b|--vblank auto|adaptive|off] [-r|--replace] [--version]\n";

typedef struct vblank_name {
    const char *name;
    wm_vblank_mode mode;
} vblank_name;

static const vblank_name vblank_names[] = {
    {"auto", WM_VBLANK_AUTO},     {"glx", WM_VBLANK_AUTO},   {"xpresent", WM_VBLANK_AUTO},
    {"adaptive", WM_VBLANK_ADAPTIVE}, {"off", WM_VBLANK_OFF},
};
static const char default_session_command[] = "xfce4-session";

static bool parse_options(int argc, char **argv, app_options *options);
static bool finish_options(app_options *options);
static const char *option_value(int argc, char **argv, int *i, const char *short_name,
                                const char *long_name);
static bool parse_compositing(const char *value, app_options *options);
static bool parse_vblank(const char *value, app_options *options);

int main(int argc, char **argv)
{
    app_options options = { 0 };
    app *app;
    int status;

    setlocale(LC_ALL, "");
    bindtextdomain("xfwm4", APP_LOCALE_DIR);
    bind_textdomain_codeset("xfwm4", "UTF-8");
    bindtextdomain(APP_TEXT_DOMAIN, APP_LOCALE_DIR);
    bind_textdomain_codeset(APP_TEXT_DOMAIN, "UTF-8");
    if (!parse_options(argc, argv, &options)) {
        fputs(usage, stderr);
        return EXIT_FAILURE;
    }
    if (options.show_version) {
        printf("xfwm-wayland %s\n", APP_VERSION);
        return EXIT_SUCCESS;
    }
    app_realtime_start();
    GPid bus = app_session_bus_start();
    app = app_create(&options);
    status = app ? app_run(app) : EXIT_FAILURE;
    if (app) {
        app_destroy(app);
    }
    app_session_bus_stop(bus);
    return status;
}

static bool is_option(const char *arg, const char *short_name, const char *long_name)
{
    return strcmp(arg, short_name) == 0 || strcmp(arg, long_name) == 0;
}

static bool parse_options(int argc, char **argv, app_options *options)
{
    for (int i = 1; i < argc; i++) {
        if (is_option(argv[i], "-V", "--version")) {
            options->show_version = true;
            continue;
        }
        if (is_option(argv[i], "-s", "--startup") && i + 1 < argc) {
            options->startup_command = argv[++i];
            continue;
        }
        if (is_option(argv[i], "-n", "--no-session")) {
            options->no_session = true;
            continue;
        }
        if (is_option(argv[i], "-r", "--replace")) {
            continue;
        }
        const char *compositing = option_value(argc, argv, &i, "-c", "--compositor");
        if (compositing != NULL) {
            if (!parse_compositing(compositing, options)) {
                return false;
            }
            continue;
        }
        const char *vblank = option_value(argc, argv, &i, "-b", "--vblank");
        if (vblank != NULL) {
            if (!parse_vblank(vblank, options)) {
                return false;
            }
            continue;
        }
        return false;
    }
    return finish_options(options);
}

static bool finish_options(app_options *options)
{
    if (options->no_session) {
        return options->startup_command == NULL;
    }
    if (options->startup_command == NULL) {
        options->startup_command = default_session_command;
    }
    return true;
}

static const char *option_value(int argc, char **argv, int *i, const char *short_name,
                                const char *long_name)
{
    const char *arg = argv[*i];
    size_t long_length = strlen(long_name);
    if (strncmp(arg, long_name, long_length) == 0 && arg[long_length] == '=') {
        return arg + long_length + 1;
    }
    if (is_option(arg, short_name, long_name) && *i + 1 < argc) {
        return argv[++*i];
    }
    return NULL;
}

static bool parse_compositing(const char *value, app_options *options)
{
    if (strcmp(value, "auto") == 0) {
        return true;
    }
    bool on = strcmp(value, "on") == 0;
    if (!on && strcmp(value, "off") != 0) {
        return false;
    }
    options->overrides.has_compositing = true;
    options->overrides.compositing = on;
    return true;
}

static bool parse_vblank(const char *value, app_options *options)
{
    for (size_t i = 0; i < sizeof(vblank_names) / sizeof(vblank_names[0]); i++) {
        if (strcmp(value, vblank_names[i].name) == 0) {
            options->overrides.has_vblank = true;
            options->overrides.vblank = vblank_names[i].mode;
            return true;
        }
    }
    return false;
}
