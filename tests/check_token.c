#include "check_nanny.h"


#include "../src/token_provider.h"
#include "../src/database.h"
#include <jansson.h>

void setup(void)
{
    initialize_database(":memory:");
    add_configuration_value("JWT_SIGN_KEY","ABC");
}

void teardown(void)
{
    close_database();
}

static
char* make_test_token() {
    user u;
    u.login = "login";
    u.enabled=1;
    u.roles_count = 1;
    const char* roles[] = {"ROLE"};
    u.roles = roles;
    u.password_hash = 0;
    return create_jwt(&u);
}

START_TEST(test_token_read)
{
    char* token = make_test_token();
    json_error_t error;
    json_auto_t* token_json = json_loads(token,0,&error);
    json_t *json_id = json_object_get(token_json,"id");    
    user* u = extract_user(json_string_value(json_id),200);
    ck_assert_ptr_nonnull(u);
    ck_assert_str_eq(u->login,"login");
    ck_assert_int_eq(u->roles_count,1);
    ck_assert_str_eq(u->roles[0],"ROLE");
    free_user(u);
    free(token);
}
END_TEST

START_TEST(test_token_read_bearer)
{
    char* token = make_test_token();
    json_error_t error;
    json_auto_t* token_json = json_loads(token,0,&error);
    json_t *json_id = json_object_get(token_json,"id");    
    const char* token_val = json_string_value(json_id);    
    size_t header_length = json_string_length(json_id) + 8;
    char* header_val = (char*)malloc( header_length );
    strncpy(header_val,"Bearer ",7);
    strncpy(header_val+7,token_val,header_length-7);
    user* u = extract_user(header_val,header_length);
    ck_assert_ptr_nonnull(u);
    ck_assert_str_eq(u->login,"login");
    ck_assert_int_eq(u->roles_count,1);
    ck_assert_str_eq(u->roles[0],"ROLE");
    free_user(u);
    free(token);
    free(header_val);
}
END_TEST

START_TEST(test_token_create)
{
    char* token = make_test_token();
    ck_assert_ptr_nonnull(token);
    json_error_t error;
    json_auto_t* token_json = json_loads(token,0,&error);
    json_t *json_id = json_object_get(token_json,"id");
    ck_assert_ptr_nonnull(json_id);
    ck_assert_int_eq(json_is_string(json_id),1);
    ck_assert_int_gt(json_string_length(json_id),0);    
    free(token);
}
END_TEST

 
Suite * tokens_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("JSON Web Tokens");

    /* Core test case */
    tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_token_create);
    tcase_add_test(tc_core, test_token_read);
    tcase_add_test(tc_core, test_token_read_bearer);
    suite_add_tcase(s, tc_core);

    return s;
}


