#ifndef STATUS_CONTROLLER_H
#define STATUS_CONTROLLER_H
#include "mongoose.h"

void handle_block_status(struct mg_connection *nc, int ev, void *ev_data);

#endif


