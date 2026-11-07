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

typedef struct arg_reader {
    int count;
    char **args;
    int index;
} arg_reader;

static void bind_text_domains(void);
static int run(const app_options *options);
static bool parse_options(int argc, char **argv, app_options *options);
static bool parse_option(arg_reader *reader, app_options *options);
static bool parse_flag(const char *arg, app_options *options);
static bool finish_options(app_options *options);
static const char *option_value(arg_reader *reader, const char *short_name,
                                const char *long_name);
static bool parse_compositing(const char *value, app_options *options);
static bool parse_vblank(const char *value, app_options *options);

int main(int argc, char **argv)
{
    app_options options = { 0 };

    bind_text_domains();
    if (!parse_options(argc, argv, &options)) {
        fputs(usage, stderr);
        return EXIT_FAILURE;
    }
    if (options.show_version) {
        printf("xfwm-wayland %s\n", APP_VERSION);
        return EXIT_SUCCESS;
    }
    return run(&options);
}

static void bind_text_domains(void)
{
    setlocale(LC_ALL, "");
    bindtextdomain("xfwm4", APP_LOCALE_DIR);
    bind_textdomain_codeset("xfwm4", "UTF-8");
    bindtextdomain(APP_TEXT_DOMAIN, APP_LOCALE_DIR);
    bind_textdomain_codeset(APP_TEXT_DOMAIN, "UTF-8");
}

static int run(const app_options *options)
{
    app_realtime_start();
    GPid bus = app_session_bus_start();
    app *app = app_create(options);
    int status = app ? app_run(app) : EXIT_FAILURE;

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
    arg_reader reader = { argc, argv, 1 };

    for (; reader.index < reader.count; reader.index++) {
        if (!parse_option(&reader, options)) {
            return false;
        }
    }
    return finish_options(options);
}

static bool parse_option(arg_reader *reader, app_options *options)
{
    const char *arg = reader->args[reader->index];

    if (parse_flag(arg, options)) {
        return true;
    }
    if (is_option(arg, "-s", "--startup") && reader->index + 1 < reader->count) {
        options->startup_command = reader->args[++reader->index];
        return true;
    }
    const char *compositing = option_value(reader, "-c", "--compositor");
    if (compositing != NULL) {
        return parse_compositing(compositing, options);
    }
    const char *vblank = option_value(reader, "-b", "--vblank");
    return vblank != NULL && parse_vblank(vblank, options);
}

static bool parse_flag(const char *arg, app_options *options)
{
    if (is_option(arg, "-V", "--version")) {
        options->show_version = true;
        return true;
    }
    if (is_option(arg, "-n", "--no-session")) {
        options->no_session = true;
        return true;
    }
    return is_option(arg, "-r", "--replace");
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

static const char *option_value(arg_reader *reader, const char *short_name,
                                const char *long_name)
{
    const char *arg = reader->args[reader->index];
    size_t long_length = strlen(long_name);
    if (strncmp(arg, long_name, long_length) == 0 && arg[long_length] == '=') {
        return arg + long_length + 1;
    }
    if (is_option(arg, short_name, long_name) && reader->index + 1 < reader->count) {
        return reader->args[++reader->index];
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
