#include "http_server.h"
#include "database.h"
#include "mongoose.h"
#include "packet_filter.h"
#include <json.h>

#include <arpa/inet.h>

static struct mg_serve_http_opts s_http_server_opts;
static int s_sig_num = 0;
static char monitoring_thread_done = 0;
static struct mg_mgr mgr;

struct name_block_data {
    struct mg_connection *nc;
    char* host;
};

static void signal_handler(int sig_num) {
  signal(sig_num, signal_handler);
  s_sig_num = sig_num;
}

/* Server handler */
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case MG_EV_HTTP_REQUEST:
        mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
      break;
    default:
      break;
  }
}

static void send_block_host_result(struct mg_connection *nc, struct in_addr address, const char* host) {
    block_address(address);
    set_host_status(host,HOST_BLOCKED);
    mg_send_head(nc,200,2,"Content-Type: text/plain");
    mg_printf(nc,"%s", "OK");
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

static void block_reverse_dns_resolve_handler(struct mg_dns_message *dns_message,void *user_data, enum mg_resolve_err err) {
    struct mg_connection *nc = (struct mg_connection *)user_data;
    if(err == MG_RESOLVE_OK) {
        for(int i=0;i<dns_message->num_answers;++i) {
            struct mg_dns_resource_record record = dns_message->answers[i];
            if(record.kind == MG_DNS_ANSWER) {
                char name[256];
                if(!mg_dns_parse_record_data(dns_message,&record,name,sizeof(name))) {
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

static void handle_block(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message *) ev_data;
    if(ev == MG_EV_HTTP_REQUEST) {
        if(strncmp("POST",hm->method.p,4) != 0) {
            mg_send_head(nc,400,12,"Content-Type: text/plain");
            mg_printf(nc,"%s", "Bad Request");
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }        
        char host[256];
        memset(host,0,sizeof(host));
        char* local_nameserver = get_configuration_value("LOCAL_NAMESERVER");
        if(mg_get_http_var(&hm->body,"Host",host,sizeof(host)) <= 0) {            
            struct mg_resolve_async_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.nameserver_url=local_nameserver;
            char reverse_name[INET_ADDRSTRLEN+15];//.in-addr.arpa
            memset(&reverse_name, 0, sizeof(reverse_name));
            const unsigned char *p = (const unsigned char *) (&nc->sa.sin.sin_addr.s_addr);
            snprintf(reverse_name,INET_ADDRSTRLEN+15,"%d.%d.%d.%d.in-addr.arpa",p[3],p[2],p[1],p[0]);
            printf("Querying %s\n",reverse_name);
            mg_resolve_async_opt(nc->mgr,reverse_name,MG_DNS_PTR_RECORD,block_reverse_dns_resolve_handler,nc,opts);
            free(local_nameserver);
        } else {
            struct mg_resolve_async_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.nameserver_url=local_nameserver;
            struct name_block_data *block_data = (struct name_block_data*) malloc(sizeof(struct name_block_data));
            block_data->nc = nc;
            block_data->host = (char*) malloc(sizeof(host));
            memset(block_data->host,0,sizeof(host));
            strncpy(block_data->host,host,sizeof(host));
            mg_resolve_async_opt(nc->mgr,host,MG_DNS_A_RECORD,block_name_resolve_handler_http,block_data,opts);
            free(local_nameserver);
        }
    }
}

static void send_allow_host_result(struct mg_connection *nc, struct in_addr address, const char* host) {
    host_status status = get_host_status(host);
    if(status == HOST_BLOCKED) {
        int usage_today = get_host_today_usage(host);
        int today_limit = get_host_today_limit(host);
        if(usage_today < today_limit) {
            unblock_address(address);
            set_host_status(host,HOST_ALLOWED);
            mg_send_head(nc,200,2,"Content-Type: text/plain");
            mg_printf(nc,"%s", "OK");
        } else {
            mg_send_head(nc,403,10,"Content-Type: text/plain");
            mg_printf(nc,"%s", "Forbidden");
        }
        nc->flags |= MG_F_SEND_AND_CLOSE;
    } else {
        mg_send_head(nc,403,10,"Content-Type: text/plain");
        mg_printf(nc,"%s", "Forbidden");
        nc->flags |= MG_F_SEND_AND_CLOSE;
    }
}

static void allow_reverse_dns_resolve_handler(struct mg_dns_message *dns_message,void *user_data, enum mg_resolve_err err) {
    struct mg_connection *nc = (struct mg_connection *)user_data;
    if(err == MG_RESOLVE_OK) {
        for(int i=0;i<dns_message->num_answers;++i) {
            struct mg_dns_resource_record record = dns_message->answers[i];
            if(record.kind == MG_DNS_ANSWER) {
                char name[256];
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

static void handle_allow(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message *) ev_data;
    if(ev == MG_EV_HTTP_REQUEST) {
        if(strncmp("POST",hm->method.p,4) != 0) {
            mg_send_head(nc,400,12,"Content-Type: text/plain");
            mg_printf(nc,"%s", "Bad Request");
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }
        char* local_nameserver = get_configuration_value("LOCAL_NAMESERVER");
        struct mg_resolve_async_opts opts;
        memset(&opts, 0, sizeof(opts));
        opts.nameserver_url=local_nameserver;
        opts.max_retries=1;
        char reverse_name[INET_ADDRSTRLEN+15];//.in-addr.arpa
        memset(&reverse_name, 0, sizeof(reverse_name));
        const unsigned char *p = (const unsigned char *) (&nc->sa.sin.sin_addr.s_addr);
        snprintf(reverse_name,INET_ADDRSTRLEN+15,"%d.%d.%d.%d.in-addr.arpa",p[3],p[2],p[1],p[0]);
        printf("Querying %s\n",reverse_name);
        mg_resolve_async_opt(nc->mgr,reverse_name,MG_DNS_PTR_RECORD,allow_reverse_dns_resolve_handler,nc,opts);
        free(local_nameserver);
    }
}

static void send_host_status(struct mg_connection *nc, const char* host) {
    host_status status = get_host_status(host);
    int usage_today = get_host_today_usage(host);
    int today_limit = get_host_today_limit(host);
    json_object* jobj = json_object_new_object();
    json_object *jhost = json_object_new_string(host);
    json_object *jstatus = json_object_new_int(status);
    json_object *jusage = json_object_new_int(usage_today);
    json_object *jlimit = json_object_new_int(today_limit);
    
    json_object_object_add(jobj,"Host", jhost);
    json_object_object_add(jobj,"Status", jstatus);
    json_object_object_add(jobj,"Usage", jusage);
    json_object_object_add(jobj,"Limit", jlimit);
    
    const char* content = json_object_to_json_string(jobj);
    mg_send_head(nc,200,strlen(content),"Content-Type: application/json");
    mg_printf(nc,"%s", content);
    nc->flags |= MG_F_SEND_AND_CLOSE;
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


static void handle_block_status(struct mg_connection *nc, int ev, void *ev_data) {
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
            opts.nameserver_url=local_nameserver;
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

static void block_name_resolve_handler(struct mg_dns_message *dns_message,void *user_data, enum mg_resolve_err err) {
    names* name = (names*)user_data;
    
    if(err == MG_RESOLVE_OK) {
        for(int i=0;i<dns_message->num_answers;++i) {
            struct mg_dns_resource_record record = dns_message->answers[i];
            if(record.kind == MG_DNS_ANSWER) {
                struct in_addr ina;
                if(!mg_dns_parse_record_data(dns_message,&record,&ina,sizeof(struct in_addr))) {
                    //printf("found ip: %s\n",inet_ntoa(ina));
                    block_address(ina);
                    set_host_status(name->name,HOST_BLOCKED);
                }
            }
        }
    }
    free(name->name);
    free(name);
}

static void block_everyone(struct mg_mgr *mgr) {
    names* name = get_host_names();
    char* local_nameserver = get_configuration_value("LOCAL_NAMESERVER");
    while(name != NULL) {
        struct mg_resolve_async_opts opts;
        memset(&opts, 0, sizeof(opts));        
        opts.nameserver_url=local_nameserver;
        //printf("Querying %s\n",name->name);
        names* next = name->next;
        mg_resolve_async_opt(mgr,name->name,MG_DNS_A_RECORD,block_name_resolve_handler,name,opts);
        name = next;
    }
    free(local_nameserver);
}

 static void *monitoring_thread_start(void *arg) {
     (void)arg;
     while(!monitoring_thread_done) {
         names* name = get_hosts_with_status(HOST_ALLOWED);
         while(name) {
            char* host = name->name;
            int usage_today = get_host_today_usage(host);
            int today_limit = get_host_today_limit(host);
            if(usage_today >= today_limit) {
                char* local_nameserver = get_configuration_value("LOCAL_NAMESERVER");
                struct mg_resolve_async_opts opts;
                memset(&opts, 0, sizeof(opts));
                opts.nameserver_url=local_nameserver;
                names* next = name->next;
                mg_resolve_async_opt(&mgr,host,MG_DNS_A_RECORD,block_name_resolve_handler,name,opts);
                name=next;
            } else {
                add_minutes_to_host_usage(host,1);
                names* tmp = name;
                name=name->next;
                free(tmp->name);
                free(tmp);
            }
         }
         if(!monitoring_thread_done) {
            sleep(60);
         }
     }
     return 0;
 }
 
int start_http_server(const http_server_options*  const options) {
    
    s_http_server_opts.enable_directory_listing = "no";
    s_http_server_opts.document_root = options->server_root;
    cs_stat_t st;
    if (mg_stat(s_http_server_opts.document_root, &st) != 0) {
        fprintf(stderr, "Cannot find %s directory, exiting\n", s_http_server_opts.document_root);
        return 1;
    }
    
    int result = 0;
    mg_mgr_init(&mgr, NULL);
    
    struct mg_connection *nc = mg_bind(&mgr, options->address, ev_handler);
    if( !nc) {
        fprintf(stderr, "Cannot bind connection on address %s\n",options->address);
        result = 1;
        goto done;
    }
    mg_set_protocol_http_websocket(nc);
    mg_register_http_endpoint(nc, "/api/block", handle_block);
    mg_register_http_endpoint(nc, "/api/allow", handle_allow);
    mg_register_http_endpoint(nc, "/api/block_status", handle_block_status);
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    block_everyone(&mgr);
    
    pthread_t monitoring_thread;
    pthread_create(&monitoring_thread,NULL,monitoring_thread_start,NULL);
    
    printf("Starting server on address %s\n", options->address);
    while (s_sig_num == 0) {
        mg_mgr_poll(&mgr, 1000);
    }
    monitoring_thread_done = 1;
    void* res;
    pthread_join(monitoring_thread,&res);
    
done:    
    mg_mgr_free(&mgr);
    return result;
}
