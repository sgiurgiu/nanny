#ifndef TOKEN_PROVIDER_H
#define TOKEN_PROVIDER_H

#include "domain.h"
#include <stdbool.h>
#include <stddef.h>

char* create_jwt(user* u);
user* extract_user(const char* token, size_t length);

#endif


