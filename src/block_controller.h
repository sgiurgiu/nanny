#ifndef BLOCK_CONTROLLER_H
#define BLOCK_CONTROLLER_H
#include "mongoose.h"

void handle_block(struct mg_connection *nc, int ev, void *ev_data);

#endif
