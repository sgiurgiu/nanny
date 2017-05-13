#ifndef DATABASE_H
#define DATABASE_H

struct names {
    char* name;
    struct names* next;
};
typedef struct names names;

int initialize_database(const char* path);
char* get_configuration_value(const char* key);
names* get_host_names();
void close_database();
#endif
