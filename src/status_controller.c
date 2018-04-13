#include "status_controller.h"
#include "database.h"
#include <jansson.h>

static void send_host_status(struct mg_connection *nc, const char* host) {
    host_status status = get_host_status(host);
    int usage_today = get_host_today_usage(host);
    int today_limit = get_host_today_limit(host);
    json_auto_t* jobj = json_object();
    
    json_object_set_new(jobj,"Host", json_string(host));
    json_object_set_new(jobj,"Status", json_integer(status));
    json_object_set_new(jobj,"Usage", json_integer(usage_today));
    json_object_set_new(jobj,"Limit", json_integer(today_limit));
    
    char* content = json_dumps(jobj,JSON_COMPACT);
    mg_send_head(nc,200,strlen(content),"Content-Type: application/json");
    mg_printf(nc,"%s", content);
    nc->flags |= MG_F_SEND_AND_CLOSE;
    free(content);
}

static void ip_resolve_handler(struct mg_dns_message *dns_message,void *user_data, enum mg_resolve_err err) {
    struct mg_connection *nc = (struct mg_connection *)user_data;
    if(err == MG_RESOLVE_OK) {
        for(int i=0;i<dns_message->num_answers;++i) {
            struct mg_dns_resource_record record = dns_message->answers[i];
            if(record.kind == MG_DNS_ANSWER) {
                char name[256];
                if(!mg_dns_parse_record_data(dns_message,&record,name,sizeof(name))) {
                    send_host_status(nc,name);
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


void handle_block_status(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message *) ev_data;
    if(ev == MG_EV_HTTP_REQUEST) {
        if(strncmp("GET",hm->method.p,3) != 0) {
            mg_send_head(nc,400,12,"Content-Type: text/plain");
            mg_printf(nc,"%s", "Bad Request");
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }

        char host[256];
        memset(host,0,sizeof(host));
        if(mg_get_http_var(&hm->body,"Host",host,sizeof(host)) <= 0) {
            char* local_nameserver = get_configuration_value("LOCAL_NAMESERVER");
            struct mg_resolve_async_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.nameserver=local_nameserver;
            char reverse_name[INET_ADDRSTRLEN+15];//.in-addr.arpa
            memset(&reverse_name, 0, sizeof(reverse_name));
            const unsigned char *p = (const unsigned char *) (&nc->sa.sin.sin_addr.s_addr);
            snprintf(reverse_name,INET_ADDRSTRLEN+15,"%d.%d.%d.%d.in-addr.arpa",p[3],p[2],p[1],p[0]);
            printf("Querying %s\n",reverse_name);
            mg_resolve_async_opt(nc->mgr,reverse_name,MG_DNS_PTR_RECORD,ip_resolve_handler,nc,opts);
            free(local_nameserver);
        } else {
            send_host_status(nc,host);
        }
    }
}

