#ifndef LOGIN_CONTROLLER_H
#define LOGIN_CONTROLLER_H
#include "mongoose.h"

void handle_login(struct mg_connection *nc, int ev, void *ev_data);

#endif

