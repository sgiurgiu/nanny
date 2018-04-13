#ifndef DOMAIN_H
#define DOMAIN_H

#include <stdbool.h>

enum host_status_t {
    HOST_BLOCKED = 0,
    HOST_ALLOWED = 1,
    HOST_UNKNOWN = -1
};
struct names {
    char* name;
    struct names* next;
};
typedef struct names names;
typedef enum host_status_t host_status;

struct user {
    char* login;
    char* password_hash;
    char** roles;    
    int roles_count;
    bool enabled;
};
typedef struct user user;

#endif

