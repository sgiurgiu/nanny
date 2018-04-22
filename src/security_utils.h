#ifndef SECURITY_UTILS_H
#define SECURITY_UTILS_H

#include "domain.h"
#include "mongoose.h"
#include <stdbool.h>
#include <stddef.h>

bool is_admin(struct mg_connection* nc, struct http_message *hm);
bool is_authenticated(struct mg_connection* nc, struct http_message *hm);

#endif



