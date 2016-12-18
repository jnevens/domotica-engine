/*
 * check_demo.c
 *
 *  Created on: Mar 16, 2015
 *      Author: jnevens
 */
#include <check.h>
#include <stdlib.h>
#include <unistd.h>

START_TEST(test_demo)
{
	char *string = calloc(1, 20);
	ck_assert_ptr_ne(string, NULL);
	free(string)
}
END_TEST

Suite * check_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("test demo");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_demo);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = check_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
