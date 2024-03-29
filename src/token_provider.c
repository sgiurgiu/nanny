#include "token_provider.h"

#include "database.h"
#include <jansson.h>
#include <jwt.h>
#include <string.h>
#include <time.h>

#define LOGIN_GRANT "login"
#define ROLES_GRANT "roles"
#define EXPIRATION_GRANT "expiration"

static char* concatenate_roles(char** roles,int roles_count) {
    int length = 0;
    for(int i=0;i<roles_count;i++) {
        length+=strlen(roles[i]);
    }
    char* concatenated_roles = (char*)malloc(length+roles_count);//to account for the commas    
    char* tmp_roles = concatenated_roles;
    for(int i=0;i<roles_count;i++) {
        int role_length=strlen(roles[i]);
        strncpy(tmp_roles,roles[i],role_length);
        if(i < (roles_count-1)) {
            tmp_roles[role_length]=',';
            tmp_roles+=role_length+1;
        }
    }
    concatenated_roles[length+roles_count-1]=0; 
    return concatenated_roles;
}

char * create_jwt(user* u) {
    char* jwt_sign_key = get_configuration_value("JWT_SIGN_KEY");
    if(!jwt_sign_key) return NULL;
    jwt_t *token;
    jwt_new(&token);
    jwt_set_alg(token,JWT_ALG_HS512,(const unsigned char*)jwt_sign_key,strlen(jwt_sign_key));
    jwt_add_grant(token,LOGIN_GRANT,u->login);
    char* roles = concatenate_roles(u->roles,u->roles_count);
    jwt_add_grant(token,ROLES_GRANT,roles);
    time_t now = time(NULL);
    jwt_add_grant_int(token,EXPIRATION_GRANT,now+86400);//expire in 24 hours (more or less)
    char* token_val = jwt_encode_str(token);
                        
    json_auto_t* jobj = json_object();
    json_object_set_new(jobj,"id", json_string(token_val));
    char* content = json_dumps(jobj,JSON_COMPACT);    
    
    free(token_val);
    free(jwt_sign_key);
    free(roles);
    jwt_free(token);    
    return content;
}

static void extract_roles(user* u, const char* roles) {
    int count = 0;
    const char* start_roles = roles;
    const char* s = roles;
    for(; *s!=0; s++) {
        if(*s == ',') count++;
    }
    if ( s -start_roles > 0) {
        u->roles_count = count + 1;
        u->roles = 0;        
    } else  {
        u->roles_count = 0;
        u->roles = 0;        
        return;
    }
    u->roles = (char**)malloc(u->roles_count * sizeof(char*));
    const char* start = roles;
    int index = 0;
    for(const char* s = roles; *s != 0; s++) {
        if(*s == ',') {
            size_t length = s - start;
            char* str = (char*) malloc(length+1);
            strncpy(str,(const char*)start,length);        
            str[length]=0;
            u->roles[index]=str;
            index++;
            start = s+1;
        }
    }
    size_t length = strlen(start);
    char* str = (char*) malloc(length+1);
    strncpy(str,(const char*)start,length);        
    str[length]=0;
    u->roles[index]=str;
    
}

user * extract_user(const char* token, size_t length) {
    
    char* jwt_sign_key = get_configuration_value("JWT_SIGN_KEY");
    jwt_t *jwt = NULL;
    size_t our_token_length = length;
    if(strncmp("Bearer ",token,7) == 0) {
        token = token + 7;
        our_token_length-=7;
    }
    char* our_token = (char*)malloc(our_token_length+1);
    strncpy(our_token,token,our_token_length);
    our_token[our_token_length]=0;
    if(jwt_decode(&jwt,our_token,(const unsigned char*)jwt_sign_key,strlen(jwt_sign_key))) {
        free(jwt_sign_key);
        free(our_token);
        return NULL;
    }
    int expiration = jwt_get_grant_int(jwt,EXPIRATION_GRANT);
    time_t now = time(NULL);
    const char* login = jwt_get_grant(jwt, LOGIN_GRANT);
    const char* roles = jwt_get_grant(jwt, ROLES_GRANT);
    if(expiration < now || !login || !roles) {
        free(jwt_sign_key);
        free(our_token);
        jwt_free(jwt);    
        return NULL;        
    }
    user* u = (user*)malloc(sizeof(user));
    size_t login_length = strlen(login)+1;
    u->login = (char*)malloc(login_length);
    strncpy(u->login,login,login_length-1);
    u->login[login_length-1] = 0;
    extract_roles(u,roles);
    u->enabled = true;
    u->password_hash = 0;
    
    free(our_token);
    free(jwt_sign_key);
    jwt_free(jwt);    
    return u;
}
