#include "channels.h"

#include <stdbool.h>

typedef struct channel_slot {
    channels *owner;
    channel_id id;
    XfconfChannel *channel;
} channel_slot;

struct channels {
    channel_watch watch;
    bool is_connected;
    channel_slot slots[CHANNEL_COUNT];
};

static bool connect_xfconf(void);
static void open_channel(channels *channels, channel_id id);
static void close_channel(channel_slot *slot);
static void on_property_changed(XfconfChannel *channel, const char *property,
                                const GValue *value, void *data);

channels *channels_create(const channel_watch *watch)
{
    channels *channels = g_new0(struct channels, 1);

    channels->watch = *watch;
    channels->is_connected = connect_xfconf();
    if (!channels->is_connected) {
        return channels;
    }
    for (int id = 0; id < CHANNEL_COUNT; id++) {
        open_channel(channels, (channel_id) id);
    }
    return channels;
}

void channels_destroy(channels *channels)
{
    for (int id = 0; id < CHANNEL_COUNT; id++) {
        close_channel(&channels->slots[id]);
    }
    if (channels->is_connected) {
        xfconf_shutdown();
    }
    g_free(channels);
}

XfconfChannel *channels_get(const channels *channels, channel_id id)
{
    return channels->slots[id].channel;
}

static bool connect_xfconf(void)
{
    GError *error = NULL;

    if (xfconf_init(&error)) {
        return true;
    }
    g_warning("Cannot connect to xfconf, using default settings: %s", error->message);
    g_error_free(error);
    return false;
}

static void open_channel(channels *channels, channel_id id)
{
    channel_slot *slot = &channels->slots[id];

    slot->owner = channels;
    slot->id = id;
    slot->channel = xfconf_channel_new(channel_name(id));
    g_signal_connect(slot->channel, "property-changed", G_CALLBACK(on_property_changed), slot);
}

static void close_channel(channel_slot *slot)
{
    if (!slot->channel) {
        return;
    }
    g_signal_handlers_disconnect_by_data(slot->channel, slot);
    g_object_unref(slot->channel);
    slot->channel = NULL;
}

static void on_property_changed(XfconfChannel *channel, const char *property,
                                const GValue *value, void *data)
{
    channel_slot *slot = data;
    channel_watch *watch = &slot->owner->watch;

    (void) channel;
    (void) value;
    watch->changed(watch->self, slot->id, property);
}
