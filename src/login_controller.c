#include "login_controller.h"
#include "http_utils.h"
#include "domain.h"
#include "database.h"
#include "token_provider.h"

#include <bcrypt.h>
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
        mg_send_head(nc,400,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Bad Request");
        nc->flags |= MG_F_SEND_AND_CLOSE; 
        return;
    }
    json_t *json_username = json_object_get(login_data,"username");
    json_t *json_password = json_object_get(login_data,"password");
    
    if(!json_is_string(json_username) || !json_is_string(json_password)) {
        mg_send_head(nc,400,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Bad Request");
        nc->flags |= MG_F_SEND_AND_CLOSE; 
        return;        
    }
    
    size_t username_length = json_string_length(json_username);
    const char* username = json_string_value(json_username);    
    user* u = get_user(username,username_length);
    if (u) {    
        fprintf(stderr,"user enabled:%d\n",u->enabled);
        fprintf(stderr,"user login:%s\n",u->login);        
        fprintf(stderr,"hashes password:%s\n",u->password_hash);
        const char* password = json_string_value(json_password);    
        int ret = bcrypt_checkpw(password, u->password_hash);
        if(ret == 0) {
            char* content = create_jwt(u);
            mg_send_head(nc,200,strlen(content),"Content-Type: application/json");
            mg_printf(nc,"%s", content);
            nc->flags |= MG_F_SEND_AND_CLOSE;
            free(content);
            free_user(u);
            return;
        } 
        free_user(u);
    } else {
        //this is to consume time even if the username is wrong/not found
        const char* hash = "$2a$12$RfdU1upmHpp3qZFZ.OyzB.fcVHzIrSW2XnW7YDuAaEokZzZq5Ldpa";
        int ret = bcrypt_checkpw("testtesttest", hash);
        if(ret == 0) {
            user* u = (user*)malloc(sizeof(user));
            u->login="not found";
            u->password_hash = (char*)hash;
            const char * roles[] = {
                    "ROLE_ADMIN",
                    "ROLE_USER"
                };      
            u->roles = (char**)roles;
            u->roles_count = 2;    
            char* content = create_jwt(u);
            fprintf(stderr,"hash :%d\n",ret);
            free(content);
            free(u);
        }
    }
        
    mg_send_head(nc,401,12,"Content-Type: text/plain");
    mg_printf(nc,"%s", "Unauthorized");
    nc->flags |= MG_F_SEND_AND_CLOSE;
}
