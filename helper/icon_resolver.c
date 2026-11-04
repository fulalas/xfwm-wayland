#include "icon_resolver.h"

#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>

static GIcon *app_icon(const char *app_id);
static GPtrArray *desktop_ids(const char *app_id);
static GIcon *icon_of_desktop_id(const char *desktop_id);
static GIcon *icon_of_search(const char *app_id);
static void free_search_results(char ***results);
static char *path_for_gicon(GIcon *icon, int size);
static char *path_for_name(const char *name, int size);
static char *path_of_info(GtkIconInfo *info);

char *icon_resolve(const char *app_id, const char *icon_name, int size)
{
    GIcon *icon = *app_id ? app_icon(app_id) : NULL;
    char *path = icon ? path_for_gicon(icon, size) : NULL;

    g_clear_object(&icon);
    if (!path && *icon_name) {
        path = path_for_name(icon_name, size);
    }
    if (!path && *app_id) {
        path = path_for_name(app_id, size);
    }
    return path;
}

static GIcon *app_icon(const char *app_id)
{
    GPtrArray *ids = desktop_ids(app_id);
    GIcon *icon = NULL;

    for (guint i = 0; i < ids->len && !icon; i++) {
        icon = icon_of_desktop_id(g_ptr_array_index(ids, i));
    }
    g_ptr_array_unref(ids);
    return icon ? icon : icon_of_search(app_id);
}

static GPtrArray *desktop_ids(const char *app_id)
{
    GPtrArray *ids = g_ptr_array_new_with_free_func(g_free);
    const char *last_dot = strrchr(app_id, '.');
    const char *last = last_dot && last_dot[1] ? last_dot + 1 : app_id;
    char *lower = g_ascii_strdown(app_id, -1);
    char *last_lower = g_ascii_strdown(last, -1);

    g_ptr_array_add(ids, g_strconcat(app_id, ".desktop", NULL));
    g_ptr_array_add(ids, g_strconcat(lower, ".desktop", NULL));
    g_ptr_array_add(ids, g_strconcat(last, ".desktop", NULL));
    g_ptr_array_add(ids, g_strconcat(last_lower, ".desktop", NULL));
    g_free(lower);
    g_free(last_lower);
    return ids;
}

static GIcon *icon_of_desktop_id(const char *desktop_id)
{
    GDesktopAppInfo *info = g_desktop_app_info_new(desktop_id);
    GIcon *icon = NULL;

    if (info) {
        icon = g_app_info_get_icon(G_APP_INFO(info));
        icon = icon ? g_object_ref(icon) : NULL;
        g_object_unref(info);
    }
    return icon;
}

static GIcon *icon_of_search(const char *app_id)
{
    char ***results = g_desktop_app_info_search(app_id);
    GIcon *icon = NULL;

    if (results && results[0] && results[0][0]) {
        icon = icon_of_desktop_id(results[0][0]);
    }
    free_search_results(results);
    return icon;
}

static void free_search_results(char ***results)
{
    for (size_t i = 0; results && results[i]; i++) {
        g_strfreev(results[i]);
    }
    g_free(results);
}

static char *path_for_gicon(GIcon *icon, int size)
{
    return path_of_info(gtk_icon_theme_lookup_by_gicon(gtk_icon_theme_get_default(), icon, size,
                                                       GTK_ICON_LOOKUP_FORCE_SIZE));
}

static char *path_for_name(const char *name, int size)
{
    return path_of_info(gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(), name, size,
                                                   GTK_ICON_LOOKUP_FORCE_SIZE));
}

static char *path_of_info(GtkIconInfo *info)
{
    char *path;

    if (!info) {
        return NULL;
    }
    path = g_strdup(gtk_icon_info_get_filename(info));
    g_object_unref(info);
    return path;
}
