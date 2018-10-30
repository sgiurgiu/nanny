#ifndef ADMIN_CONTROLLER_H
#define ADMIN_CONTROLLER_H
#include "mongoose.h"

void handle_admin_hosts(struct mg_connection *nc, int ev, void *ev_data);
void handle_admin_host_history(struct mg_connection *nc, int ev, void *ev_data);
void handle_admin_add_host_time(struct mg_connection *nc, int ev, void *ev_data);
void handle_is_admin(struct mg_connection *nc, int ev, void *ev_data);
void handle_admin_update_password(struct mg_connection *nc, int ev, void *ev_data);
#endif


