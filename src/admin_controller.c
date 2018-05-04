#include "admin_controller.h"
#include "domain.h"
#include "database.h"
#include "security_utils.h"
#include "http_utils.h"
#include "domain.h"

#include <string.h>
#include <jansson.h>
#include <sclog4c/sclog4c.h>

void handle_admin_host_history(struct mg_connection *nc, int ev, void *ev_data) {
    if(ev != MG_EV_HTTP_REQUEST) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        return;
    }    
    if(!ensure_get_request(nc,ev,ev_data)) return;
    struct http_message *hm = (struct http_message *) ev_data;
    if(!is_admin(nc,hm) ) {
        return;
    }
    char host[256] = {0};    
    char date[256] = {0};    
    char end_date[256]={0};
    if(mg_get_http_var(&hm->query_string,"host",host,sizeof(host)-1) <= 0 ||
       mg_get_http_var(&hm->query_string,"start_date",date,sizeof(date)-1) <= 0 ) {
        mg_send_head(nc,400,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Bad Request");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }
    int end_date_length = mg_get_http_var(&hm->query_string,"end_date",end_date,sizeof(end_date)-1);
    json_auto_t* jarray = json_array();
    logm(SL4C_DEBUG, "ADMIN: Getting host history for host %s from date %s until date %s",host,date,(end_date_length <= 0 ? "" : end_date));    
    host_usage* usage = get_host_usage(host,date,end_date_length <= 0 ? NULL : end_date);
    while(usage) {
        json_t* jobj = json_object();
        json_object_set_new(jobj,"host", json_string(usage->name));
        json_object_set_new(jobj,"day", json_string(usage->day));
        json_object_set_new(jobj,"minutes", json_integer(usage->minutes));        
        json_array_append_new(jarray,jobj);
        
        host_usage* next = usage->next;
        free(usage->name);
        free(usage->day);    
        free(usage);
        usage = next;
    }
    
    char* content = json_dumps(jarray,JSON_COMPACT);
    mg_send_head(nc,200,strlen(content),"Content-Type: application/json");
    mg_printf(nc,"%s", content);
    nc->flags |= MG_F_SEND_AND_CLOSE;
    free(content);
}

void handle_admin_hosts(struct mg_connection* nc, int ev, void* ev_data) {
    if(ev != MG_EV_HTTP_REQUEST) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        return;
    }    
    if(!ensure_get_request(nc,ev,ev_data)) return;
    
    struct http_message *hm = (struct http_message *) ev_data;
    if(!is_admin(nc,hm) ) {
        return;
    }
    json_auto_t* jarray = json_array();
    logm(SL4C_DEBUG, "ADMIN: Getting hosts list");    
    names* name = get_host_names();
    while(name) {        
        host_status status = get_host_status(name->name);
        int limit = get_host_today_limit(name->name);
        int usage = get_host_today_usage(name->name);
        if(limit > 0 && usage < 0) {
            usage = 0;
        }        
        
        json_t* jobj = json_object();
        json_object_set_new(jobj,"host", json_string(name->name));
        json_object_set_new(jobj,"status", json_integer(status));
        json_object_set_new(jobj,"usage", json_integer(usage));
        json_object_set_new(jobj,"limit", json_integer(limit));        
        json_array_append_new(jarray,jobj);
        
        names* next = name->next;
        free(name->name);
        free(name);        
        name = next;
    }
    
    
    char* content = json_dumps(jarray,JSON_COMPACT);
    mg_send_head(nc,200,strlen(content),"Content-Type: application/json");
    mg_printf(nc,"%s", content);
    nc->flags |= MG_F_SEND_AND_CLOSE;
    free(content);
}
void handle_is_admin(struct mg_connection *nc, int ev, void *ev_data) {
    if(ev != MG_EV_HTTP_REQUEST) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        return;
    }    
    if(!ensure_get_request(nc,ev,ev_data)) return;
    logm(SL4C_DEBUG, "ADMIN: Is Admin?");    
    struct http_message *hm = (struct http_message *) ev_data;
    if(!is_admin(nc,hm) ) {
        logm(SL4C_DEBUG, "ADMIN: Is Admin? NAH");    
        return;
    }
    mg_send_head(nc,200,2,"Content-Type: application/json");
    mg_printf(nc,"%s", "OK");
    nc->flags |= MG_F_SEND_AND_CLOSE;    
}

void handle_admin_add_host_time(struct mg_connection *nc, int ev, void *ev_data) {
    if(ev != MG_EV_HTTP_REQUEST) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        return;
    }    
    if(!ensure_post_request(nc,ev,ev_data)) return;
    
    struct http_message *hm = (struct http_message *) ev_data;
    if(!is_admin(nc,hm) ) {
        return;
    }
    
    json_error_t json_error;
    json_auto_t *host_data = json_loadb(hm->body.p,  hm->body.len, 0, &json_error);
    if(!host_data) {
        fprintf(stderr, "json error on line %d: %s\n", json_error.line, json_error.text);
        mg_send_head(nc,400,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Bad Request");
        nc->flags |= MG_F_SEND_AND_CLOSE; 
        return;
    }
    json_t *json_host = json_object_get(host_data,"host");
    json_t *json_minutes = json_object_get(host_data,"minutes");
    if(!json_is_string(json_host) || (!json_is_integer(json_minutes) && !json_is_string(json_minutes))) {
        mg_send_head(nc,400,12,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Bad Request");
        nc->flags |= MG_F_SEND_AND_CLOSE; 
        return;        
    }
    const char* host = json_string_value(json_host);    
    int minutes = 0;
    if(json_is_integer(json_minutes)) {
        minutes = json_integer_value(json_minutes);
    } else {
        minutes = atoi(json_string_value(json_minutes));
    }
    logm(SL4C_DEBUG, "ADMIN: Add host %s minutes %d",host,minutes);    
    add_minutes_to_host_limit(host,minutes);
    
    char* content = json_host_status(host);
    mg_send_head(nc,200,strlen(content),"Content-Type: application/json");
    mg_printf(nc,"%s", content);
    nc->flags |= MG_F_SEND_AND_CLOSE;
    free(content);
}

