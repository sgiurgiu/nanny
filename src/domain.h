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

struct host_usage {
    char* name;
    char* day;
    int minutes;
    struct host_usage* next;
};

typedef struct host_usage host_usage;

struct user {
    char* login;
    char* password_hash;
    char** roles;    
    int roles_count;
    bool enabled;
};
typedef struct user user;

void free_user(user* u);
char* json_host_status(const char* host);

#endif

