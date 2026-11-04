#include "edid.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wlr/backend.h>
#include <wlr/backend/drm.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>
#include <xf86drmMode.h>

static const char edid_property[] = "EDID";

static edid_blob read_if_edid(int fd, uint32_t property_id, uint64_t value);
static edid_blob read_blob(int fd, uint32_t blob_id);
static bool write_all(int fd, const edid_blob *blob);

edid_blob edid_read(struct wlr_output *output)
{
    edid_blob blob = {0};
    if (!wlr_output_is_drm(output)) {
        return blob;
    }
    int fd = wlr_backend_get_drm_fd(output->backend);
    drmModeObjectProperties *properties =
        fd >= 0 ? drmModeObjectGetProperties(fd, wlr_drm_connector_get_id(output),
                                             DRM_MODE_OBJECT_CONNECTOR)
                : NULL;
    for (uint32_t i = 0; properties != NULL && i < properties->count_props; i++) {
        if (blob.data == NULL) {
            blob = read_if_edid(fd, properties->props[i], properties->prop_values[i]);
        }
    }
    drmModeFreeObjectProperties(properties);
    return blob;
}

void edid_release(edid_blob *blob)
{
    free(blob->data);
    *blob = (edid_blob){0};
}

int edid_create_fd(const edid_blob *blob)
{
    int fd = memfd_create("edid", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0) {
        wlr_log_errno(WLR_ERROR, "cannot create a file for the EDID");
        return -1;
    }
    if (!write_all(fd, blob)) {
        wlr_log_errno(WLR_ERROR, "cannot write the EDID");
        close(fd);
        return -1;
    }
    fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_SEAL);
    return fd;
}

static edid_blob read_if_edid(int fd, uint32_t property_id, uint64_t value)
{
    drmModePropertyRes *property = drmModeGetProperty(fd, property_id);
    bool is_edid = property != NULL && strcmp(property->name, edid_property) == 0 && value != 0;
    drmModeFreeProperty(property);
    return is_edid ? read_blob(fd, (uint32_t) value) : (edid_blob){0};
}

static edid_blob read_blob(int fd, uint32_t blob_id)
{
    edid_blob blob = {0};
    drmModePropertyBlobRes *source = drmModeGetPropertyBlob(fd, blob_id);
    if (source != NULL && source->length > 0) {
        blob.data = malloc(source->length);
        blob.size = blob.data != NULL ? source->length : 0;
    }
    if (blob.data != NULL) {
        memcpy(blob.data, source->data, blob.size);
    }
    drmModeFreePropertyBlob(source);
    return blob;
}

static bool write_all(int fd, const edid_blob *blob)
{
    size_t written = 0;
    while (written < blob->size) {
        ssize_t count = write(fd, blob->data + written, blob->size - written);
        if (count <= 0) {
            return false;
        }
        written += (size_t) count;
    }
    return true;
}
