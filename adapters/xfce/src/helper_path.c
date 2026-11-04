#include "helper_path.h"

#include <glib.h>

static const char helper_name[] = "xfwm-wayland-helper";
static const char build_tree_dir[] = "../helper";
static const char install_path[] = XFCE_HELPER_INSTALL_PATH;

static char *executable_dir(void);
static char *first_executable(const GPtrArray *candidates);

char *helper_path_find(void)
{
    GPtrArray *candidates = g_ptr_array_new_with_free_func(g_free);
    char *dir = executable_dir();
    char *found;

    if (dir) {
        g_ptr_array_add(candidates, g_build_filename(dir, helper_name, NULL));
        g_ptr_array_add(candidates, g_build_filename(dir, build_tree_dir, helper_name, NULL));
    }
    g_ptr_array_add(candidates, g_strdup(install_path));
    found = first_executable(candidates);
    g_ptr_array_unref(candidates);
    g_free(dir);
    return found;
}

static char *executable_dir(void)
{
    char *executable = g_file_read_link("/proc/self/exe", NULL);
    char *dir = executable ? g_path_get_dirname(executable) : NULL;

    g_free(executable);
    return dir;
}

static char *first_executable(const GPtrArray *candidates)
{
    for (guint i = 0; i < candidates->len; i++) {
        const char *path = g_ptr_array_index(candidates, i);
        if (g_file_test(path, G_FILE_TEST_IS_EXECUTABLE)) {
            return g_strdup(path);
        }
    }
    return NULL;
}
