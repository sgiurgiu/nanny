#ifndef ADMIN_CONTROLLER_H
#define ADMIN_CONTROLLER_H
#include "mongoose.h"

void handle_admin_hosts(struct mg_connection *nc, int ev, void *ev_data);

#endif


