#include "database.h"
#include "sqlite3.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static sqlite3 *db = NULL;
pthread_mutex_t host_usage_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t host_status_mtx = PTHREAD_MUTEX_INITIALIZER;

int initialize_database(const char* path) {
    #define ERRCHECK(msg) {if (rc != SQLITE_OK) {fprintf(stderr, "%s: %s\n",msg, sqlite3_errmsg(db));  sqlite3_close(db); return rc;}}   
    
    int rc = sqlite3_open(path,&db);
    ERRCHECK("Can't open database");    
    rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS CONFIGURATION(KEY TEXT,VALUE TEXT)",NULL,NULL,NULL);
    ERRCHECK("Can't execute query");
    rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS HOST_ALLOWANCE(NAME TEXT,DAY INT, MINUTES INT)",NULL,NULL,NULL);
    ERRCHECK("Can't execute query");
    rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS HOST_USAGE(NAME TEXT,DAY TEXT, MINUTES INT)",NULL,NULL,NULL);
    ERRCHECK("Can't execute query");
    rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS HOST_STATUS(NAME TEXT,STATUS INT)",NULL,NULL,NULL);
    ERRCHECK("Can't execute query");
    rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS HOST_ALLOWANCE_EXT(NAME TEXT,DAY TEXT, MINUTES INT)",NULL,NULL,NULL);
    ERRCHECK("Can't execute query");
    rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS USERS(ID INTEGER PRIMARY KEY,LOGIN TEXT,PASSWORD_HASH TEXT, ENABLED INT)",NULL,NULL,NULL);
    ERRCHECK("Can't execute query");
    rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS ROLES(ID INTEGER PRIMARY KEY, ROLE_NAME TEXT)",NULL,NULL,NULL);
    ERRCHECK("Can't execute query");
    rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS USER_ROLES(USER_ID INTEGER, ROLE_ID INTEGER,  "
                      "FOREIGN KEY(USER_ID) REFERENCES USERS(ID), FOREIGN KEY(ROLE_ID) REFERENCES ROLES(ID) )",NULL,NULL,NULL);
    ERRCHECK("Can't execute query");
    
    #undef ERRCHECK    
    return rc;
}

static char* get_string(sqlite3_stmt *stmt, int col) {
    const unsigned char *value = sqlite3_column_text(stmt,col);
    int length = sqlite3_column_bytes(stmt,0);
    char* str = (char*) malloc(length+1);
    strncpy(str,(const char*)value,length);        
    str[length]=0;
    return str;
}

int add_user(const user* u) {
    return 0;
}

int get_users_count() {
    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(db,"SELECT COUNT(*) FROM USERS",-1,&stmt,0) != SQLITE_OK) {
        fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));  
        return -1;
    }
    
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return count;
    }
    sqlite3_finalize(stmt);  
    return -1;
}

user * get_user(const char* username, size_t username_length) {
    sqlite3_stmt *stmt;
    
    if(sqlite3_prepare_v2(db,"SELECT ID,LOGIN,PASSWORD_HASH,ENABLED FROM USERS WHERE LOGIN=?",-1,&stmt,0) != SQLITE_OK) {
        fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    if(sqlite3_bind_text(stmt,1,username,username_length,NULL) != SQLITE_OK) {
        fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }
    
    if(sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return NULL;
    }
    user* u = (user*)malloc(sizeof(user));
    int id = sqlite3_column_int(stmt, 0);
    u->login = get_string(stmt,1);
    u->password_hash = get_string(stmt,2);
    u->enabled = sqlite3_column_int(stmt, 3) != 0;
    sqlite3_finalize(stmt);
    
    
    
    return u;
}

static int get_host_today_limit_ext(const char* host) {
    sqlite3_stmt *stmt;
    
    if(sqlite3_prepare_v2(db,"SELECT MINUTES FROM HOST_ALLOWANCE_EXT WHERE NAME=? AND DAY=date('now','localtime')",-1,&stmt,0) != SQLITE_OK) {
        fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    if(sqlite3_bind_text(stmt,1,host,-1,NULL) != SQLITE_OK) {
        fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }
    int minutes = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        minutes += sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return minutes;
}

int get_host_today_limit(const char* host) {
    sqlite3_stmt *stmt;
    
    if(sqlite3_prepare_v2(db,"SELECT MINUTES FROM HOST_ALLOWANCE WHERE NAME=? AND DAY=strftime('%w','now','localtime')",-1,&stmt,0) != SQLITE_OK) {
        fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    if(sqlite3_bind_text(stmt,1,host,-1,NULL) != SQLITE_OK) {
        fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        int minutes = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return minutes + get_host_today_limit_ext(host);
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

int add_minutes_to_host_usage(const char* host, int minutes) {
    
    int current_minutes = get_host_today_usage(host);
    
    if(current_minutes < 0) {
        sqlite3_stmt *stmt;
        pthread_mutex_lock(&host_usage_mtx);
        if(sqlite3_prepare_v2(db,"INSERT INTO HOST_USAGE (NAME,MINUTES,DAY) VALUES(?,?,date('now','localtime'))",-1,&stmt,0) != SQLITE_OK) {
            fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));
            pthread_mutex_unlock(&host_usage_mtx);
            return -1;
        }
        if(sqlite3_bind_text(stmt,1,host,-1,NULL) != SQLITE_OK) {
            fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&host_usage_mtx);
            return -1;
        }
        if(sqlite3_bind_int(stmt,2,minutes) != SQLITE_OK) {
            fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&host_usage_mtx);
            return -1;
        }
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&host_usage_mtx);
        return minutes;
    } else {
        sqlite3_stmt *stmt;
        pthread_mutex_lock(&host_usage_mtx);
        if(sqlite3_prepare_v2(db,"UPDATE HOST_USAGE SET MINUTES=? WHERE NAME=? AND DAY=date('now','localtime')",-1,&stmt,0) != SQLITE_OK) {
            fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));
            pthread_mutex_unlock(&host_usage_mtx);
            return -1;
        }
        if(sqlite3_bind_int(stmt,1,current_minutes+minutes) != SQLITE_OK) {
            fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&host_usage_mtx);
            return -1;
        }
        if(sqlite3_bind_text(stmt,2,host,-1,NULL) != SQLITE_OK) {
            fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&host_usage_mtx);
            return -1;
        }
        sqlite3_step(stmt);        
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&host_usage_mtx);
        return current_minutes+minutes;
    }
}

names* get_hosts_with_status(host_status status) {
    sqlite3_stmt *stmt;
//     pthread_mutex_lock(&host_status_mtx);
    if(sqlite3_prepare_v2(db,"SELECT DISTINCT NAME FROM HOST_STATUS WHERE STATUS=?",-1,&stmt,0) != SQLITE_OK) {
        fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));  
        pthread_mutex_unlock(&host_status_mtx);
        return NULL;
    }
    if(sqlite3_bind_int(stmt,1,status) != SQLITE_OK) {
        fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&host_status_mtx);
        return NULL;
    }
    names* root = NULL;
    names* name = NULL;
    while(sqlite3_step(stmt) != SQLITE_DONE) {
        const unsigned char * value = sqlite3_column_text(stmt, 0);
        int length = sqlite3_column_bytes(stmt,0);
        if(root == NULL) {
            root = (names*)malloc(sizeof(names));
            root->name = NULL;
            root->next = NULL;
            name = root;
        } else {
            names* new_name = (names*)malloc(sizeof(names));
            new_name->name = NULL;
            new_name->next = NULL;
            name->next = new_name;
            name = new_name;
        }
        name->name = (char*) malloc(length+1);
        memset(name->name,0,length+1);
        strncpy(name->name,(const char*)value,length);        
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&host_status_mtx);
    return root;
}

int get_host_today_usage(const char* host) {
    sqlite3_stmt *stmt;
    pthread_mutex_lock(&host_usage_mtx);
    if(sqlite3_prepare_v2(db,"SELECT MINUTES FROM HOST_USAGE WHERE NAME=? AND DAY=date('now','localtime')",-1,&stmt,0) != SQLITE_OK) {
        fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));
        pthread_mutex_unlock(&host_usage_mtx);
        return -1;
    }
    if(sqlite3_bind_text(stmt,1,host,-1,NULL) != SQLITE_OK) {
        fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&host_usage_mtx);
        return -1;
    }
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        int minutes = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&host_usage_mtx);
        return minutes;
    }
    
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&host_usage_mtx);
    return -1;
}

int set_host_status(const char* host,host_status status) {
    host_status existing_status = get_host_status(host);
    pthread_mutex_lock(&host_status_mtx);
    if(existing_status == HOST_UNKNOWN) {
        sqlite3_stmt *stmt;
        if(sqlite3_prepare_v2(db,"INSERT INTO HOST_STATUS (STATUS,NAME) VALUES (?,?)",-1,&stmt,0) != SQLITE_OK) {
            fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));  
            pthread_mutex_unlock(&host_status_mtx);
            return 1;
        }
        if(sqlite3_bind_int(stmt,1,status) != SQLITE_OK) {
            fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&host_status_mtx);
            return -1;
        }
        if(sqlite3_bind_text(stmt,2,host,-1,NULL) != SQLITE_OK) {
            fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&host_status_mtx);
            return 1;
        }
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    } else {
        sqlite3_stmt *stmt;
        if(sqlite3_prepare_v2(db,"UPDATE HOST_STATUS SET STATUS=? WHERE NAME=?",-1,&stmt,0) != SQLITE_OK) {
            fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));  
            pthread_mutex_unlock(&host_status_mtx);
            return 1;
        }
        if(sqlite3_bind_int(stmt,1,status) != SQLITE_OK) {
            fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&host_status_mtx);
            return -1;
        }
        if(sqlite3_bind_text(stmt,2,host,-1,NULL) != SQLITE_OK) {
            fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&host_status_mtx);
            return 1;
        }
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    pthread_mutex_unlock(&host_status_mtx);
    return 0;
}
host_status get_host_status(const char* host) {
    sqlite3_stmt *stmt;
    pthread_mutex_lock(&host_status_mtx);
    if(sqlite3_prepare_v2(db,"SELECT STATUS FROM HOST_STATUS WHERE NAME=?",-1,&stmt,0) != SQLITE_OK) {
        fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));  
        pthread_mutex_unlock(&host_status_mtx);
        return HOST_UNKNOWN;
    }
    if(sqlite3_bind_text(stmt,1,host,-1,NULL) != SQLITE_OK) {
        fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));        
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&host_status_mtx);
        return HOST_UNKNOWN;
    }
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        host_status status = (host_status)sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&host_status_mtx);
        return status;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&host_status_mtx);
    fprintf(stderr, "Cannot get host status for %s. It is not found.\n", host);        
    return HOST_UNKNOWN;
}

char* get_configuration_value(const char* key) {
    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(db,"SELECT VALUE FROM CONFIGURATION WHERE KEY=?",-1,&stmt,0) != SQLITE_OK) {
        fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));  
        return NULL;
    }
    if(sqlite3_bind_text(stmt,1,key,-1,SQLITE_STATIC) != SQLITE_OK) {
        fprintf(stderr, "Cannot bind key: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return NULL;
    }
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char * value = sqlite3_column_text(stmt, 0);
        int length = sqlite3_column_bytes(stmt, 0);
        char* value_copy = (char*) malloc(length+1);
        memset(value_copy,0,length+1);
        strncpy(value_copy,(const char*)value,length);
        sqlite3_finalize(stmt);
        return value_copy;
    }
    
    sqlite3_finalize(stmt);
    return NULL;
}

names* get_host_names() {
    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(db,"SELECT DISTINCT NAME FROM HOST_ALLOWANCE",-1,&stmt,0) != SQLITE_OK) {
        fprintf(stderr, "Cannot execute query: %s\n", sqlite3_errmsg(db));  
        return NULL;
    }
    names* root = NULL;
    names* name = NULL;
    while(sqlite3_step(stmt) != SQLITE_DONE) {
        const unsigned char * value = sqlite3_column_text(stmt, 0);
        int length = sqlite3_column_bytes(stmt, 0);      
        if(root == NULL) {
            root = (names*)malloc(sizeof(names));
            root->name = NULL;
            root->next = NULL;
            name = root;
        } else {
            names* new_name = (names*)malloc(sizeof(names));
            new_name->name = NULL;
            new_name->next = NULL;
            name->next = new_name;
            name = new_name;
        }
        name->name = (char*) malloc(length+1);
        memset(name->name,0,length+1);
        strncpy(name->name,(const char*)value,length);        
        printf("Found host name %s\n",name->name);
    }
    sqlite3_finalize(stmt);
    
    return root;
}


void close_database() {
    if(db) {
        sqlite3_close(db);
        db = NULL;
    }
    pthread_mutex_destroy(&host_usage_mtx);
    pthread_mutex_destroy(&host_status_mtx);
}
