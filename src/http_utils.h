#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H
#include "mongoose.h"
#include <stdbool.h>

bool ensure_post_request(struct mg_connection *nc, int ev, void *ev_data);
bool ensure_get_request(struct mg_connection *nc, int ev, void *ev_data);
bool ensure_delete_request(struct mg_connection *nc, int ev, void *ev_data);

#endif


