#include "admin_controller.h"
#include "domain.h"
#include "database.h"
#include "token_provider.h"

#include <string.h>

static bool is_admin(struct mg_connection* nc, struct http_message *hm) {
    struct mg_str* auth_value = mg_get_http_header(hm,"Authorization");
    if(auth_value == NULL) {
        mg_send_head(nc,401,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Unauthorized");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return false;
    }
    user* u = extract_user(auth_value->p,auth_value->len);
    if(u == NULL) {
        mg_send_head(nc,401,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Unauthorized");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return false;
    }
    bool isAdmin = false;
    for(int i=0;i<u->roles_count;i++) {
        if(strncmp("ROLE_ADMIN",u->roles[i],11)) {
            isAdmin = true;
            break;
        }
    }
    if(!isAdmin) {
        mg_send_head(nc,403,10,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Forbidden");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        free_user(u);
        return false;
    }
    free_user(u);
    return true;
}

void handle_admin_hosts(struct mg_connection* nc, int ev, void* ev_data) {
    if(ev != MG_EV_HTTP_REQUEST) {
        return;
    }    
    struct http_message *hm = (struct http_message *) ev_data;
    if(!is_admin(nc,hm) ) {
        return;
    }
    
    mg_send_head(nc,200,2,"Content-Type: text/plain");
    mg_printf(nc,"%s", "OK");
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

