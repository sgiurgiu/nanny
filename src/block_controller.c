#include "block_controller.h"
#include "database.h"
#include "packet_filter.h"
#include "http_utils.h"
#include "domain.h"

#include <arpa/inet.h>
#include <jansson.h>

struct name_block_data {
    struct mg_connection *nc;
    char* host;
};


static void send_block_host_result(struct mg_connection *nc, struct in_addr address, const char* host) {
    block_address(address);
    set_host_status(host,HOST_BLOCKED);
    char* content = json_host_status(host);
    mg_send_head(nc,200,strlen(content),"Content-Type: application/json");
    mg_printf(nc,"%s", content);
    nc->flags |= MG_F_SEND_AND_CLOSE;
    free(content);
}

static void block_name_resolve_handler_http(struct mg_dns_message *dns_message,void *user_data, enum mg_resolve_err err) {
    struct name_block_data *block_data = (struct name_block_data*) user_data;
    
    if(err == MG_RESOLVE_OK) {
        for(int i=0;i<dns_message->num_answers;++i) {
            struct mg_dns_resource_record record = dns_message->answers[i];
            if(record.kind == MG_DNS_ANSWER) {
                struct in_addr ina;
                if(!mg_dns_parse_record_data(dns_message,&record,&ina,sizeof(struct in_addr))) {
                    //printf("found ip: %s\n",inet_ntoa(ina));
                    send_block_host_result(block_data->nc,ina,block_data->host);
                }
            }
        }
    }
    free(block_data->host);
    free(block_data);
}


static void block_reverse_dns_resolve_handler(struct mg_dns_message *dns_message,void *user_data, enum mg_resolve_err err) {
    struct mg_connection *nc = (struct mg_connection *)user_data;
    if(err == MG_RESOLVE_OK) {
        for(int i=0;i<dns_message->num_answers;++i) {
            struct mg_dns_resource_record record = dns_message->answers[i];
            if(record.kind == MG_DNS_ANSWER) {
                char name[256] = {0};
                if(!mg_dns_parse_record_data(dns_message,&record,name,sizeof(name)-1)) {
                    send_block_host_result(nc,nc->sa.sin.sin_addr,name);
                } else {
                    mg_send_head(nc,500,14,"Content-Type: text/plain");
                    mg_printf(nc, "Server Error");
                    nc->flags |= MG_F_SEND_AND_CLOSE;
                }
            }
        }
    } else {
        mg_send_head(nc,500,14,"Content-Type: text/plain");
        mg_printf(nc, "Server Error");
        nc->flags |= MG_F_SEND_AND_CLOSE;
    }
}

void handle_block(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message *) ev_data;
    if(!ensure_post_request(nc,ev,ev_data)) return;
    
    char* local_nameserver = get_configuration_value("LOCAL_NAMESERVER");
    struct mg_resolve_async_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.nameserver=local_nameserver;        
    
    if(hm->body.len <= 0) {      
        //if we got called without any payload as to what host to block, block the caller
        char reverse_name[INET_ADDRSTRLEN+15];//.in-addr.arpa
        memset(&reverse_name, 0, sizeof(reverse_name));
        const unsigned char *p = (const unsigned char *) (&nc->sa.sin.sin_addr.s_addr);
        snprintf(reverse_name,INET_ADDRSTRLEN+15,"%d.%d.%d.%d.in-addr.arpa",p[3],p[2],p[1],p[0]);
        printf("Querying %s\n",reverse_name);
        mg_resolve_async_opt(nc->mgr,reverse_name,MG_DNS_PTR_RECORD,block_reverse_dns_resolve_handler,nc,opts);
    } else {
        
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
        if(!json_is_string(json_host)) {
            mg_send_head(nc,400,12,"Content-Type: text/plain");
            mg_printf(nc,"%s", "Bad Request");
            nc->flags |= MG_F_SEND_AND_CLOSE; 
            return;        
        }
        size_t host_length = json_string_length(json_host);
        const char* host = json_string_value(json_host);            
        struct name_block_data *block_data = (struct name_block_data*) malloc(sizeof(struct name_block_data));
        block_data->nc = nc;
        block_data->host = (char*) malloc(host_length);
        memset(block_data->host,0,host_length);
        strncpy(block_data->host,host,host_length);
        mg_resolve_async_opt(nc->mgr,host,MG_DNS_A_RECORD,block_name_resolve_handler_http,block_data,opts);
    }
    free(local_nameserver);
    
}
