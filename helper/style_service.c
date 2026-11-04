#include "style_service.h"

#include <gtk/gtk.h>

#include "icon_resolver.h"
#include "protocol_io.h"
#include "style_report.h"

typedef struct style_service {
    GtkWidget *widget;
    guint resend_id;
} style_service;

static void watch_style(style_service *service);
static void on_style_changed(void *instance, void *data);
static void on_setting_changed(GObject *settings, GParamSpec *spec, void *data);
static gboolean on_resend(void *data);
static void on_icon(void *self, const icon_query *query);
static void on_closed(void *self);

int style_service_run(void)
{
    style_service service = { .widget = gtk_window_new(GTK_WINDOW_POPUP) };
    request_handler handler = { .self = &service, .icon = on_icon, .closed = on_closed };

    watch_style(&service);
    style_report_send(service.widget);
    protocol_listen(&handler);
    gtk_main();
    gtk_widget_destroy(service.widget);
    return 0;
}

static void watch_style(style_service *service)
{
    GtkSettings *settings = gtk_settings_get_default();

    g_signal_connect(settings, "notify::gtk-theme-name", G_CALLBACK(on_setting_changed), service);
    g_signal_connect(settings, "notify::gtk-font-name", G_CALLBACK(on_setting_changed), service);
    g_signal_connect(service->widget, "style-updated", G_CALLBACK(on_style_changed), service);
}

static void on_style_changed(void *instance, void *data)
{
    style_service *service = data;

    (void) instance;
    if (!service->resend_id) {
        service->resend_id = g_idle_add(on_resend, service);
    }
}

static void on_setting_changed(GObject *settings, GParamSpec *spec, void *data)
{
    (void) spec;
    on_style_changed(settings, data);
}

static gboolean on_resend(void *data)
{
    style_service *service = data;

    service->resend_id = 0;
    style_report_send(service->widget);
    return G_SOURCE_REMOVE;
}

static void on_icon(void *self, const icon_query *query)
{
    char *path = query->size > 0 ? icon_resolve(query->app_id, query->icon_name, query->size)
                                 : NULL;
    char id[G_ASCII_DTOSTR_BUF_SIZE];

    (void) self;
    g_snprintf(id, sizeof(id), "%u", query->id);
    protocol_write("icon", id, path ? path : "");
    g_free(path);
}

static void on_closed(void *self)
{
    (void) self;
    gtk_main_quit();
}
