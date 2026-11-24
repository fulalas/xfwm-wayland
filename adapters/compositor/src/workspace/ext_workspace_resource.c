#include "ext_workspace_resource.h"

#if HAVE_WLR_EXT_WORKSPACE_FROM_RESOURCE
#include <wlr/types/wlr_ext_workspace_v1.h>
#else
#include <string.h>

static const char WORKSPACE_INTERFACE[] = "ext_workspace_handle_v1";

typedef struct wlroots_workspace_resource {
    struct wl_resource *resource;
    struct wlr_ext_workspace_handle_v1 *workspace;
} wlroots_workspace_resource;
#endif

typedef struct resource_search {
    const struct wlr_ext_workspace_handle_v1 *workspace;
    struct wl_resource *found;
} resource_search;

static enum wl_iterator_result match_resource(struct wl_resource *resource, void *data);

struct wlr_ext_workspace_handle_v1 *ext_workspace_from_resource(struct wl_resource *resource)
{
#if HAVE_WLR_EXT_WORKSPACE_FROM_RESOURCE
    return wlr_ext_workspace_handle_v1_from_resource(resource);
#else
    if (strcmp(wl_resource_get_class(resource), WORKSPACE_INTERFACE) != 0) {
        return NULL;
    }
    const wlroots_workspace_resource *data = wl_resource_get_user_data(resource);
    return data != NULL && data->resource == resource ? data->workspace : NULL;
#endif
}

struct wl_resource *ext_workspace_resource_of_client(
    struct wl_client *client, const struct wlr_ext_workspace_handle_v1 *workspace)
{
    resource_search search = {workspace, NULL};
    if (workspace != NULL) {
        wl_client_for_each_resource(client, match_resource, &search);
    }
    return search.found;
}

static enum wl_iterator_result match_resource(struct wl_resource *resource, void *data)
{
    resource_search *search = data;
    if (ext_workspace_from_resource(resource) != search->workspace) {
        return WL_ITERATOR_CONTINUE;
    }
    search->found = resource;
    return WL_ITERATOR_STOP;
}
