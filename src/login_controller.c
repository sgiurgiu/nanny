#include "login_controller.h"
#include "http_utils.h"
#include "domain.h"
#include "database.h"

#include <jansson.h>

void handle_login(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message *) ev_data;
    if(!ensure_post_request(nc,ev,ev_data)) return;
    if(hm->body.len <= 0) {
        mg_send_head(nc,400,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Bad Request");
        nc->flags |= MG_F_SEND_AND_CLOSE; 
        return;        
    }
    json_error_t json_error;
    json_auto_t *login_data = json_loadb(hm->body.p,  hm->body.len, 0, &json_error);
    if(!login_data) {
        fprintf(stderr, "json error on line %d: %s\n", json_error.line, json_error.text);
        return;
    }
    json_auto_t *json_username = json_object_get(login_data,"username");
    json_auto_t *json_password = json_object_get(login_data,"password");
    
    if(!json_is_string(json_username) || !json_is_string(json_password)) {
        mg_send_head(nc,400,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Bad Request");
        nc->flags |= MG_F_SEND_AND_CLOSE; 
        return;        
    }
    size_t username_length = json_string_length(json_username);
    size_t password_length = json_string_length(json_password);
    const char* username = json_string_value(json_username);
    const char* password = json_string_value(json_password);
    
    
    
    mg_send_head(nc,200,2,"Content-Type: text/plain");
    mg_printf(nc,"%s", "OK");
    nc->flags |= MG_F_SEND_AND_CLOSE;
    
}
