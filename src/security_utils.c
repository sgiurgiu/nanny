#include "security_utils.h"
#include "token_provider.h"

user* get_authenticated_user (struct http_message *hm) {
    struct mg_str* auth_value = mg_get_http_header(hm,"Authorization");
    if(auth_value == NULL) {
        return NULL;
    }
    return extract_user(auth_value->p,auth_value->len);
}

bool is_authenticated(struct mg_connection* nc, struct http_message *hm) {
    user* u = get_authenticated_user(hm);
    if(u == NULL) {
        mg_send_head(nc,401,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Unauthorized");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return false;
    }

    bool authenticated = u->roles_count > 0;
    if(!authenticated) {
        mg_send_head(nc,403,10,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Forbidden");
        nc->flags |= MG_F_SEND_AND_CLOSE;
    }    
    free_user(u);
    return authenticated;
}

bool is_admin(struct mg_connection* nc, struct http_message *hm) {
    user* u = get_authenticated_user(hm);
    if(u == NULL) {
        mg_send_head(nc,401,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Unauthorized");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return false;
    }
    
    bool admin = false;
    for(int i=0;i<u->roles_count;i++) {
        if(strncmp("ROLE_ADMIN",u->roles[i],11)) {
            admin = true;
            break;
        }
    }
    if(!admin) {
        mg_send_head(nc,403,10,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Forbidden");
        nc->flags |= MG_F_SEND_AND_CLOSE;
    }
    free_user(u);
    return admin;
}
