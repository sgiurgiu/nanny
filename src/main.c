#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <bcrypt.h>

#include "mongoose.h"
#include "http_server.h"
#include "database.h"
#include "packet_filter.h"

static const char *db_file = "db/nanny.db";


static void print_usage(char* name) {
    printf("Usage: %s [-a [ip:]port] [-r <folder>] [-d|--db <db_file>] [-D]\n",name);
    printf("\t -a [ip:]port  Specifies the IP and the port the server should listen on. Eg. 127.0.0.1. Default 8000\n");
    printf("\t -r <folder>   Specifies the document root folder. Where index.html is located. Default '.'\n");
    printf("\t -d|--db <db_file> Database location. Default 'db/nanny.db'\n");
    printf("\t -D Daemonize.\n");
    printf("\t -h|--help This help message.\n");
}


static int check_configuration() {
    char* pf_table_name = get_configuration_value("PF_TABLE_NAME");
    if(!pf_table_name) {
        fprintf(stderr, "PF_TABLE_NAME option is not defined\n");
        return 1;
    }
    free(pf_table_name);
    char* local_nameserver = get_configuration_value("LOCAL_NAMESERVER");
    if(!local_nameserver) {
        fprintf(stderr, "LOCAL_NAMESERVER option is not defined\n");
        return 1;
    }
    free(local_nameserver);
    char* local_domain = get_configuration_value("LOCAL_DOMAIN");
    if(!local_domain) {
        fprintf(stderr, "LOCAL_DOMAIN option is not defined\n");
        return 1;
    }
    free(local_domain);
    char* jwt_sign_key = get_configuration_value("JWT_SIGN_KEY");
    if(!jwt_sign_key) {
        fprintf(stderr, "JWT_SIGN_KEY option is not defined\n");
        return 1;
    }
    free(jwt_sign_key);    
    return 0;
}

static void initialize_database_roles() {
    if(get_roles_count() > 0) return;
    const char * roles[] = {
        "ROLE_ADMIN",
        "ROLE_USER"
    };    
    add_roles(roles,2);
}
static void initialize_database_users() {
    if(get_users_count() > 0) return;
 	char salt[BCRYPT_HASHSIZE];
 	char hash[BCRYPT_HASHSIZE];
 	int ret;
 
 	ret = bcrypt_gensalt(12, salt);
 	assert(ret == 0);
 	ret = bcrypt_hashpw("admin", salt, hash);
 	assert(ret == 0);
     
    user* u = (user*)malloc(sizeof(user));
    u->login = "admin";
    u->password_hash = hash;
    u->enabled = true;
    const char * roles[] = {
        "ROLE_ADMIN",
        "ROLE_USER"
    };      
    u->roles = roles;
    u->roles_count = 2;    
    
    add_user(u);
    
    free(u);
}

int main(int argc, char **argv) {
    int i;

    http_server_options options;
    options.server_root = ".";
    options.address = "127.0.0.1:8000";
    int daemonize = 0;
    
    for (i = 1; i < argc; i++) {
        if( strncmp(argv[i], "-h", 2) == 0 || 
            strncmp(argv[i], "--help", 6) == 0) {
            print_usage(argv[0]);
            return -1;
        } else if (strncmp(argv[i], "-a", 2) == 0 && i + 1 < argc) {
            options.address = argv[++i];
        } else if (strncmp(argv[i], "-r", 2) == 0 && i + 1 < argc) {
            options.server_root = argv[++i];
        } else if ((strncmp(argv[i], "-d", 2) == 0 && i + 1 < argc) || (strncmp(argv[i], "--db", 4) == 0 && i + 1 < argc)) {
            db_file = argv[++i];
        } else if(strncmp(argv[i], "-D", 2) == 0) {
            daemonize = 1;
        }
    }
    
    if(initialize_database(db_file)) {
        return -1;
    }
    initialize_database_roles();
    initialize_database_users();
    
    if(check_configuration()) {
        goto done;
    }
    
    if(daemonize) {
        if(daemon(0,1)) {
            fprintf(stderr,"Cannot daemonize\n");
        }
    }
    
    if(start_http_server(&options)) {
        goto done;
    }
    
done:
    close_database();
    
    return 0;
}
