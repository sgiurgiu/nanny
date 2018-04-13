#include "http_utils.h"

static const struct mg_str s_get_method = MG_MK_STR("GET");
static const struct mg_str s_post_method = MG_MK_STR("POST");
static const struct mg_str s_delete_method = MG_MK_STR("DELETE");

static int is_equal(const struct mg_str *s1, const struct mg_str *s2) {
  return s1->len == s2->len && memcmp(s1->p, s2->p, s2->len) == 0;
}

static bool ensure_method_request(struct mg_connection *nc, int ev, void *ev_data,const struct mg_str* s_method ) {
    if(ev != MG_EV_HTTP_REQUEST) {
        return false;
    }
    struct http_message *hm = (struct http_message *) ev_data;
    if(!is_equal(&hm->method, s_method)) {
        mg_send_head(nc,400,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Bad Request");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return false;
    }           
        
    return true;    
}

bool ensure_post_request(struct mg_connection *nc, int ev, void *ev_data) {
    return ensure_method_request(nc,ev,ev_data,&s_post_method);
}
bool ensure_get_request(struct mg_connection *nc, int ev, void *ev_data) {
    return ensure_method_request(nc,ev,ev_data,&s_get_method);
}
bool ensure_delete_request(struct mg_connection *nc, int ev, void *ev_data) {
    return ensure_method_request(nc,ev,ev_data,&s_delete_method);
}
