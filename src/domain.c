#include "domain.h"
#include <stdlib.h>

void free_user(user* u) {
    free(u->login);
    free(u->password_hash);
    if(u->roles_count > 0) {
        for(int i=0;i<u->roles_count;i++) {
            free(u->roles[i]);
        }
        free(u->roles);
    }
    free(u);
}

