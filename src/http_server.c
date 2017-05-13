#include "http_server.h"
#include "database.h"
#include "mongoose.h"
#include "packet_filter.h"

#include <arpa/inet.h>

static struct mg_serve_http_opts s_http_server_opts;
static int s_sig_num = 0;

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

static void handle_block(struct mg_connection *nc, int ev, void *ev_data) {
   (void) ev; (void) ev_data;
   mg_printf(nc, "HTTP/1.0 200 OK\r\n\r\n[I am Hello1]");
  nc->flags |= MG_F_SEND_AND_CLOSE;
}

static void handle_allow(struct mg_connection *nc, int ev, void *ev_data) {
  (void) ev; (void) ev_data;
   mg_printf(nc, "HTTP/1.0 200 OK\r\n\r\n[I am Hello2]");
  nc->flags |= MG_F_SEND_AND_CLOSE;
}

static void resolve_handler(struct mg_dns_message *dns_message,void *user_data, enum mg_resolve_err err) {
    names* name = (names*)user_data;
    
    if(err == MG_RESOLVE_OK) {
        for(int i=0;i<dns_message->num_answers;++i) {
            struct mg_dns_resource_record record = dns_message->answers[0];
            if(record.kind == MG_DNS_ANSWER) {
                struct in_addr ina;
                if(!mg_dns_parse_record_data(dns_message,&record,&ina,sizeof(struct in_addr))) {
                    //printf("found ip: %s\n",inet_ntoa(ina));
                    block_address(ina);
                }
            }
        }
    }
    free(name->name);
    free(name);
}

static void block_everyone(struct mg_mgr *mgr) {
    names* name = get_host_names();
    while(name != NULL) {
        struct mg_resolve_async_opts opts;
        memset(&opts, 0, sizeof(opts));
        opts.nameserver_url="udp://192.168.1.1:53";
        //printf("Querying %s\n",name->name);
        mg_resolve_async_opt(mgr,name->name,MG_DNS_A_RECORD,resolve_handler,name,opts);
        name = name->next;
    }
}

int start_http_server(const http_server_options*  const options) {
    struct mg_mgr mgr;
    s_http_server_opts.enable_directory_listing = "no";
    s_http_server_opts.document_root = options->server_root;
    int result = 0;
    mg_mgr_init(&mgr, NULL);
    
    struct mg_connection *nc = mg_bind(&mgr, options->address, ev_handler);
    if( !nc) {
        fprintf(stderr, "Cannot bind connection on address %s\n",options->address);
        result = 1;
        goto done;
    }
    mg_set_protocol_http_websocket(nc);
    mg_register_http_endpoint(nc, "/block", handle_block);
    mg_register_http_endpoint(nc, "/allow", handle_allow);
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    block_everyone(&mgr);
    
    printf("Starting server on address %s\n", options->address);
    while (s_sig_num == 0) {
        mg_mgr_poll(&mgr, 1000);
    }

done:    
    mg_mgr_free(&mgr);
    return result;
}
