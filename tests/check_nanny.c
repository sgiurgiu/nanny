#include <check.h>
#include <stdlib.h>

#include "check_nanny.h"

Suite * master_suite(void)
{
    Suite *s;

    s = suite_create("Master");
    return s;
}


int main(void)
{
    int number_failed;
    SRunner *sr;
    sr = srunner_create(master_suite());
    srunner_add_suite(sr,tokens_suite());
    srunner_set_fork_status (sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
   return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
