#ifndef DATABASE_H
#define DATABASE_H

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
#endif
