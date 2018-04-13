#include "token_provider.h"

#include "database.h"
#include <jansson.h>
#include <jwt.h>
#include <string.h>
#include <time.h>

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
    jwt_t *token;
    jwt_new(&token);
    char* jwt_sign_key = get_configuration_value("JWT_SIGN_KEY");
    jwt_set_alg(token,JWT_ALG_HS512,(const unsigned char*)jwt_sign_key,strlen(jwt_sign_key));
    jwt_add_grant(token,"subject",u->login);
    char* roles = concatenate_roles(u->roles,u->roles_count);
    jwt_add_grant(token,"roles",roles);
    time_t now = time(NULL);
    jwt_add_grant_int(token,"expiration",now+86400);//expire in 24 hours (more or less)
    char* token_val = jwt_encode_str(token);
                        
    json_auto_t* jobj = json_object();
    json_auto_t *jhost = json_string(token_val);
    json_object_set(jobj,"id", jhost);
    char* content = json_dumps(jobj,JSON_COMPACT);    
    
    free(token_val);
    free(jwt_sign_key);
    free(roles);
    jwt_free(token);    
    return content;
}

user * extract_user(const char* token, size_t length) {
    return NULL;
}
