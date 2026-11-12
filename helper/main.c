#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "style_service.h"
#include "terminate_dialog.h"

static const char terminate_flag[] = "--terminate";

int main(int argc, char **argv)
{
    if (!gtk_init_check(NULL, NULL)) {
        fprintf(stderr, "xfwm-wayland-helper: cannot open the display\n");
        return EXIT_FAILURE;
    }
    if (argc > 1 && strcmp(argv[1], terminate_flag) == 0) {
        return terminate_dialog_run(argv + 1);
    }
    return style_service_run();
}
