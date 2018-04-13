#include "block_controller.h"
#include "database.h"
#include "packet_filter.h"
#include "http_utils.h"

#include <arpa/inet.h>

struct name_block_data {
    struct mg_connection *nc;
    char* host;
};


static void send_block_host_result(struct mg_connection *nc, struct in_addr address, const char* host) {
    block_address(address);
    set_host_status(host,HOST_BLOCKED);
    mg_send_head(nc,200,2,"Content-Type: text/plain");
    mg_printf(nc,"%s", "OK");
    nc->flags |= MG_F_SEND_AND_CLOSE;
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
    
    char host[256] = {0};    
    char* local_nameserver = get_configuration_value("LOCAL_NAMESERVER");
    struct mg_resolve_async_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.nameserver=local_nameserver;        
    
    if(mg_get_http_var(&hm->body,"Host",host,sizeof(host)-1) <= 0) {      
        //if we got called without any payload as to what host to block, block the caller
        char reverse_name[INET_ADDRSTRLEN+15];//.in-addr.arpa
        memset(&reverse_name, 0, sizeof(reverse_name));
        const unsigned char *p = (const unsigned char *) (&nc->sa.sin.sin_addr.s_addr);
        snprintf(reverse_name,INET_ADDRSTRLEN+15,"%d.%d.%d.%d.in-addr.arpa",p[3],p[2],p[1],p[0]);
        printf("Querying %s\n",reverse_name);
        mg_resolve_async_opt(nc->mgr,reverse_name,MG_DNS_PTR_RECORD,block_reverse_dns_resolve_handler,nc,opts);
    } else {
        struct name_block_data *block_data = (struct name_block_data*) malloc(sizeof(struct name_block_data));
        block_data->nc = nc;
        block_data->host = (char*) malloc(sizeof(host));
        memset(block_data->host,0,sizeof(host));
        strncpy(block_data->host,host,sizeof(host));
        mg_resolve_async_opt(nc->mgr,host,MG_DNS_A_RECORD,block_name_resolve_handler_http,block_data,opts);
    }
    free(local_nameserver);
    
}
