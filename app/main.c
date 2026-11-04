#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"

static const char usage[] =
    "Usage: xfwm-wayland [-s|--startup COMMAND] [--version]\n";

static bool parse_options(int argc, char **argv, app_options *options);

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
    app = app_create(&options);
    if (!app) {
        return EXIT_FAILURE;
    }
    status = app_run(app);
    app_destroy(app);
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
        return false;
    }
    return true;
}
