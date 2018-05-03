#include "domain.h"
#include <stdlib.h>
#include <jansson.h>
#include "database.h"

void free_user(user* u) {
    free(u->login);
    free(u->password_hash);
    if(u->roles_count > 0) {
        for(int i=0;i<u->roles_count;i++) {
            free(u->roles[i]);
        }
        free(u->roles);
    }
    free(u);
}

char* json_host_status(const char* host) {
    host_status status = get_host_status(host);
    int usage_today = get_host_today_usage(host);
    int today_limit = get_host_today_limit(host);
    if(today_limit > 0 && usage_today < 0) {
        usage_today = 0;
    }
    json_auto_t* jobj = json_object();
    
    json_object_set_new(jobj,"host", json_string(host));
    json_object_set_new(jobj,"status", json_integer(status));
    json_object_set_new(jobj,"usage", json_integer(usage_today));
    json_object_set_new(jobj,"limit", json_integer(today_limit));
    
    return json_dumps(jobj,JSON_COMPACT);    
}
