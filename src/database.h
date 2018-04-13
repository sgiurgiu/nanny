#ifndef DATABASE_H
#define DATABASE_H

#include "domain.h"
#include <stddef.h>

int initialize_database(const char* path);
char* get_configuration_value(const char* key);
names* get_host_names();
names* get_hosts_with_status(host_status status);
host_status get_host_status(const char* host);
int set_host_status(const char* host,host_status status);
void close_database();
int get_host_today_usage(const char* host);
int add_minutes_to_host_usage(const char* host, int minutes);
int get_host_today_limit(const char* host);
user* get_user(const char* username,size_t username_length);
int add_user(const user* u);
int add_roles(const char** roles,int count);
int get_users_count();
int get_roles_count();

#endif
