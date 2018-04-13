#include "allow_controller.h"
#include "database.h"
#include "packet_filter.h"
#include "http_utils.h"

static void send_allow_host_result(struct mg_connection *nc, struct in_addr address, const char* host) {
    host_status status = get_host_status(host);
    if( (status == HOST_BLOCKED) && 
        (get_host_today_usage(host) < get_host_today_limit(host)) ) {
        unblock_address(address);
        set_host_status(host,HOST_ALLOWED);
        mg_send_head(nc,200,2,"Content-Type: text/plain");
        mg_printf(nc,"%s", "OK");
    } else {
        mg_send_head(nc,403,10,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Forbidden");
    }
    
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

static void allow_reverse_dns_resolve_handler(struct mg_dns_message *dns_message,void *user_data, enum mg_resolve_err err) {
    struct mg_connection *nc = (struct mg_connection *)user_data;
    if(err == MG_RESOLVE_OK) {
        for(int i=0;i<dns_message->num_answers;++i) {
            struct mg_dns_resource_record record = dns_message->answers[i];
            if(record.kind == MG_DNS_ANSWER) {
                char name[256] = {0};
                if(!mg_dns_parse_record_data(dns_message,&record,name,sizeof(name))) {
                    send_allow_host_result(nc,nc->sa.sin.sin_addr,name);
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

void handle_allow(struct mg_connection *nc, int ev, void *ev_data) {    
    if(!ensure_post_request(nc,ev,ev_data)) return;
    
    char* local_nameserver = get_configuration_value("LOCAL_NAMESERVER");
    struct mg_resolve_async_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.nameserver=local_nameserver;
    opts.max_retries=1;
    char reverse_name[INET_ADDRSTRLEN+15];//.in-addr.arpa
    memset(&reverse_name, 0, sizeof(reverse_name));
    const unsigned char *p = (const unsigned char *) (&nc->sa.sin.sin_addr.s_addr);
    snprintf(reverse_name,INET_ADDRSTRLEN+15,"%d.%d.%d.%d.in-addr.arpa",p[3],p[2],p[1],p[0]);
    printf("Querying %s\n",reverse_name);
    mg_resolve_async_opt(nc->mgr,reverse_name,MG_DNS_PTR_RECORD,allow_reverse_dns_resolve_handler,nc,opts);
    free(local_nameserver);
}
