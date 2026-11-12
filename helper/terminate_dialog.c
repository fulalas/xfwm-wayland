#define GETTEXT_PACKAGE "xfwm4"

#include "terminate_dialog.h"

#include <glib/gi18n-lib.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include <stdio.h>

static GtkWidget *create_dialog(const char *title);

int terminate_dialog_run(char **argv)
{
    char *title = g_strjoinv(" ", argv + 1);
    GtkWidget *dialog;
    bool terminate;

    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    dialog = create_dialog(title);
    terminate = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES;
    gtk_widget_destroy(dialog);
    g_free(title);
    puts(terminate ? "yes" : "no");
    return 0;
}

static GtkWidget *create_dialog(const char *title)
{
    GtkWidget *dialog = gtk_message_dialog_new(
        NULL, 0, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, "%s",
        _("This window might be busy and is not responding.\n"
          "Do you want to terminate the application?"));

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO);
    g_object_set(G_OBJECT(dialog), "secondary-text", title, NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), _("Warning"));
    return dialog;
}
