#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"

static const char usage[] =
    "Usage: xfwm-wayland [-s|--startup COMMAND | -n|--no-session] [--version]\n";
static const char default_session_command[] = "xfce4-session";

static bool parse_options(int argc, char **argv, app_options *options);
static bool finish_options(app_options *options);

int main(int argc, char **argv)
{
    app_options options = { 0 };
    app *app;
    int status;

    setlocale(LC_ALL, "");
    bindtextdomain("xfwm4", APP_LOCALE_DIR);
    bind_textdomain_codeset("xfwm4", "UTF-8");
    if (!parse_options(argc, argv, &options)) {
        fputs(usage, stderr);
        return EXIT_FAILURE;
    }
    if (options.show_version) {
        printf("xfwm-wayland %s\n", APP_VERSION);
        return EXIT_SUCCESS;
    }
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
