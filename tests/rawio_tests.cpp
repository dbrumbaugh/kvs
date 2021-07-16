#include <check.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "io/rawio.hpp"
#include "io/exceptions.hpp"
#include <fcntl.h>

using namespace std;

const char *test_file = "./tests/data/testfile.store";
const char *fail_file = "./tests/data/failfile.store";


START_TEST(create_succeed)
{
    IOHandler *test;
    bool error = false;
    size_t bs = 4 * 1024;

    try {
        test = new RawIOHandler(test_file, bs);
    } catch (IOException& e) {
        error = true;
    }

    ck_assert_int_eq(error, false);
    ck_assert_int_ne(test->get_fd(), 0);
    ck_assert_int_eq(test->get_bs(), bs);

    delete test;
}
END_TEST


START_TEST(create_fail)
{
    size_t bs = 4 * 1024;
    bool error = false;

    try {
        new RawIOHandler(fail_file, bs);
    } catch (IOException& e) {
        error = true;
    }

    ck_assert_int_eq(error, true);
}
END_TEST


START_TEST(destroy)
{
    IOHandler *test;
    size_t bs = 4 * 1024;

    test = new RawIOHandler(test_file, bs);

    fd_t fd = test->get_fd();

    delete test;

    // verify that fd is no longer valid

    int valid = fcntl(fd, F_GETFD);
    ck_assert_int_eq(valid, -1);
    ck_assert_int_eq(errno, EBADF);
}
END_TEST


Suite *test_suite()
{
    Suite *suite = suite_create("RawIO Tests");

    // Test the basic functionality
    TCase *basic = tcase_create("basic");
    tcase_add_test(basic, create_succeed);
    tcase_add_test(basic, create_fail);
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
