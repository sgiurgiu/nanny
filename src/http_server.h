#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

struct http_server_options {
    char* address;
    char* server_root;
};

typedef struct http_server_options http_server_options;

int start_http_server(const http_server_options*  const options);
    
#endif
