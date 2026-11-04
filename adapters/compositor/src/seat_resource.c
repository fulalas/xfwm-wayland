#include <wlr/types/wlr_seat.h>

#include "seat.h"
#include "seat_resource.h"

bool seat_resource_is_ours(const compositor *server, struct wl_resource *seat)
{
    struct wlr_seat_client *seat_client = wlr_seat_client_from_resource(seat);
    return seat_client != NULL && server->seat != NULL &&
           seat_client->seat == seat_wlr(server->seat);
}
