#include <check.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "io/mem.hpp"
#include "io/exceptions.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <climits>

using namespace std;


START_TEST(create_succeed)
{
    IOHandler *test;
    bool error = false;

    try {
        test = new MemIOHandler(1024);
    } catch (IOException& e) {
        error = true;
    }

    ck_assert_int_eq(error, false);
    ck_assert_int_eq(test->get_fd(), 0);
    ck_assert_int_eq(test->get_flen(), 0);

    delete test;
}
END_TEST


START_TEST(destroy)
{
    IOHandler *test;

    test = new MemIOHandler(1024);

    delete test;
}
END_TEST


START_TEST(write_test)
{
    IOHandler *test;
    bool error = false;
    int count = 0;
    const int buffsize = 45;

    test = new MemIOHandler(1024);

    char *test_data = new char[buffsize];
    strncpy(test_data, "hello world 1 2 3 4 5", buffsize);

    ck_assert_int_eq(test->get_flen(), 0);

    try {
        count = test->write(test_data, buffsize, 0);
    } catch (IOException& e) {
        error = true;
    }

    ck_assert_int_eq(error, false);
    ck_assert_int_eq(count, buffsize);
    ck_assert_int_eq(test->get_flen(), buffsize);
}
END_TEST


START_TEST(write_hole)
{
    IOHandler *test;
    bool error = false;
    int count = 0;
    const int buffsize = 45;

    test = new MemIOHandler(1024);

    ck_assert_int_eq(test->get_flen(), 0);
    char *test_data = new char[buffsize];
    strncpy(test_data, "more hello world stuff. Yay!\n", buffsize);

    try {
        count = test->write(test_data, buffsize, 100);
    } catch (IOException& e) {
        error = true;
    }

    ck_assert_int_eq(error, false);
    ck_assert_int_eq(count, buffsize);
    ck_assert_int_eq(test->get_flen(), buffsize + 100);
}
END_TEST


START_TEST(write_read)
{
    IOHandler *test;
    bool error = false;
    int count = 0;

    const int buffsize = 45;

    test = new MemIOHandler(1024);

    char *write_buffer = new char[buffsize];
    strncpy(write_buffer, "hello world 1 2 3 4 5", buffsize);

    ck_assert_int_eq(test->get_flen(), 0);

    try {
        count = test->write(write_buffer, buffsize, 0);
    } catch (IOException& e) {
        error = true;
    }

    ck_assert_int_eq(error, false);
    ck_assert_int_eq(count, buffsize);
    ck_assert_int_eq(test->get_flen(), buffsize);

    byte *read_buffer = new byte[buffsize];

    try {
        count = test->read(read_buffer, buffsize, 0);
    } catch (IOException& e) {
        error = true;
    }

    ck_assert_int_eq(error, false);
    ck_assert_int_eq(count, buffsize);

    int match = memcmp(write_buffer, read_buffer, buffsize);
    ck_assert_int_eq(match, 0);
}
END_TEST


Suite *test_suite()
{
    Suite *suite = suite_create("RawIO Tests");

    // Test the basic functionality
    TCase *basic = tcase_create("basic");
    tcase_add_test(basic, create_succeed);
    //tcase_add_test(basic, create_fail);

    tcase_add_test(basic, write_test);
    tcase_add_test(basic, write_hole);
    tcase_add_test(basic, write_read);

    tcase_add_test(basic, destroy);

    // TODO: Add stress testing
    TCase *stress = tcase_create("stress");

    suite_add_tcase(suite, basic);
    suite_add_tcase(suite, stress);

    return suite;
}


int run_test_suite()
{
    int failed = 0;
    Suite *suite = test_suite();
    SRunner *runner = srunner_create(suite);

    srunner_run_all(runner, CK_VERBOSE);
    failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return failed;
}


int main()
{
    int failed = run_test_suite();

    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
