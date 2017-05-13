#include "database.h"
#include "sqlite3.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static sqlite3 *db = NULL;

int initialize_database(const char* path)
{
    #define ERRCHECK(msg) {if (rc != SQLITE_OK) {fprintf(stderr, "%s: %s\n",msg, sqlite3_errmsg(db));  sqlite3_close(db); return rc;}}   
    
    int rc = sqlite3_open(path,&db);
    ERRCHECK("Can't open database");    
    rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS CONFIGURATION(KEY TEXT,VALUE TEXT)",NULL,NULL,NULL);
    ERRCHECK("Can't execute query");
    rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS HOST_ALLOWANCE(NAME TEXT,DAY INT, MINUTES INT)",NULL,NULL,NULL);
    ERRCHECK("Can't execute query");
    rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS HOST_USAGE(NAME TEXT,DAY TEXT, MINUTES INT)",NULL,NULL,NULL);
    ERRCHECK("Can't execute query");
    
    return rc;
}

char* get_configuration_value(const char* key)
{
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
        size_t length = strlen((const char*)value);
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
        size_t length = strlen((const char*)value);        
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
    
    return root;
}


void close_database()
{
    if(db) {
        sqlite3_close(db);
        db = NULL;
    }
}
