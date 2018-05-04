#include "http_server.h"
#include "database.h"
#include "mongoose.h"
#include "packet_filter.h"
#include "block_controller.h"
#include "allow_controller.h"
#include "status_controller.h"
#include "login_controller.h"
#include "admin_controller.h"

#include <sclog4c/sclog4c.h>
#include <stdbool.h>

static struct mg_serve_http_opts s_http_server_opts;
static int s_sig_num = 0;
static bool monitoring_thread_done = false;
static pthread_mutex_t wait_monitoring_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wait_monitoring_cv = PTHREAD_COND_INITIALIZER;
 
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

static void block_name_resolve_handler(struct mg_dns_message *dns_message,void *user_data, enum mg_resolve_err err) {
    names* name = (names*)user_data;
    
    if(err == MG_RESOLVE_OK) {
        for(int i=0;i<dns_message->num_answers;++i) {
            struct mg_dns_resource_record record = dns_message->answers[i];
            if(record.kind == MG_DNS_ANSWER) {
                struct in_addr ina;
                if(!mg_dns_parse_record_data(dns_message,&record,&ina,sizeof(struct in_addr))) {
                    logm(SL4C_DEBUG, "found ip %s",inet_ntoa(ina)); 
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
        opts.nameserver=local_nameserver;
        logm(SL4C_DEBUG, "blocking %s",name->name); 
        names* next = name->next;
        mg_resolve_async_opt(mgr,name->name,MG_DNS_A_RECORD,block_name_resolve_handler,name,opts);
        name = next;
    }
    free(local_nameserver);
}

 static void *monitoring_thread_start(void *arg) {
     struct mg_mgr* mgr = (struct mg_mgr*)arg;
     
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
                opts.nameserver=local_nameserver;
                names* next = name->next;
                mg_resolve_async_opt(mgr,host,MG_DNS_A_RECORD,block_name_resolve_handler,name,opts);
                name=next;
            } else {
                logm(SL4C_DEBUG, "add minutes to host usage"); 
                add_minutes_to_host_usage(host,1);
                names* tmp = name;
                name=name->next;
                free(tmp->name);
                free(tmp);
            }
         }

         struct timespec wait_time;
         wait_time.tv_sec = time(NULL) + 60;
         wait_time.tv_nsec = 0;
         pthread_mutex_lock(&wait_monitoring_mutex);
         while(!monitoring_thread_done) {
            logm(SL4C_DEBUG, "wait for time to pass"); 
            
            int ret = pthread_cond_timedwait(&wait_monitoring_cv,&wait_monitoring_mutex,&wait_time);            
            if(ret == ETIMEDOUT || difftime(time(NULL),wait_time.tv_sec) >= 60) break;
         }
         pthread_mutex_unlock(&wait_monitoring_mutex);
     }
     return 0;
 }
 
int start_http_server(const http_server_options*  const options) {
    
    s_http_server_opts.enable_directory_listing = "no";
    s_http_server_opts.document_root = options->server_root;
    cs_stat_t st;
    if (mg_stat(s_http_server_opts.document_root, &st) != 0) {
        logm(SL4C_ERROR, "Cannot find %s directory, exiting", s_http_server_opts.document_root);  
        return 1;
    }    
    struct mg_mgr mgr;
    int result = 0;
    mg_mgr_init(&mgr, NULL);
    
    struct mg_connection *nc = mg_bind(&mgr, options->address, ev_handler);
    if( !nc) {
        logm(SL4C_ERROR, "Cannot bind connection on address %s",options->address);          
        result = 1;
        goto done;
    }
    
    mg_set_protocol_http_websocket(nc);
    mg_register_http_endpoint(nc, "/api/block", handle_block);
    mg_register_http_endpoint(nc, "/api/allow", handle_allow);
    mg_register_http_endpoint(nc, "/api/block_status", handle_block_status);
    mg_register_http_endpoint(nc, "/api/login", handle_login);
    mg_register_http_endpoint(nc, "/api/admin", handle_is_admin);
    mg_register_http_endpoint(nc, "/api/admin/hosts", handle_admin_hosts);
    mg_register_http_endpoint(nc, "/api/admin/host_history", handle_admin_host_history);
    mg_register_http_endpoint(nc, "/api/admin/add_host_time", handle_admin_add_host_time);
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    block_everyone(&mgr);
    
    pthread_t monitoring_thread;
    pthread_create(&monitoring_thread,NULL,monitoring_thread_start,&mgr);
    
    logm(SL4C_INFO, "Starting server on address %s",options->address);    
    while (s_sig_num == 0) {
        mg_mgr_poll(&mgr, 1000);
    }
    monitoring_thread_done = true;
    
    pthread_mutex_lock(&wait_monitoring_mutex);
    pthread_cond_signal(&wait_monitoring_cv);
    pthread_mutex_unlock(&wait_monitoring_mutex);
    void* res;
    pthread_join(monitoring_thread,&res);
    
    pthread_mutex_destroy(&wait_monitoring_mutex);
    pthread_cond_destroy(&wait_monitoring_cv);    
done:    
    mg_mgr_free(&mgr);
    return result;
}
