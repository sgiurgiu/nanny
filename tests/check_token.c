#include <stdlib.h>
#include <check.h>
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
    suite_add_tcase(s, tc_core);

    return s;
}


int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = tokens_suite();
    sr = srunner_create(s);
    srunner_set_fork_status (sr, CK_NOFORK);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
   return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
