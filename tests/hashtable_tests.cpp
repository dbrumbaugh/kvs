#include <check.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <climits>
#include <vector>

#include "dstruct/hashtable.hpp"

using namespace std;


START_TEST(create)
{
    auto test = new HashTable<int32_t, int32_t>(100);

    ck_assert_int_eq(test->get_bucket_count(), 100);

    delete test;
}
END_TEST


START_TEST(destroy)
{
    auto test = new HashTable<int32_t, int32_t>(100);

    delete test;
}
END_TEST


START_TEST(insert)
{
    auto test = new HashTable<int32_t, int32_t>(10);

    int32_t keys[10] = {101, 201, 301, 401, 501, 601, 701, 801, 109, 110};
    size_t n = 10;
    for (size_t i=0; i<n; i++) {
        int32_t val = rand();
        auto x = test->insert(keys[i], val);
        ck_assert_int_eq(x, val);
    }


    delete test;
}
END_TEST


START_TEST(read_test)
{
    auto test = new HashTable<int32_t, int32_t>(10);

    auto to_insert = new std::vector<std::pair<int32_t, int32_t>>();

    size_t n = 1000;
    for (size_t i=0; i<n; i++) {
        int32_t key = rand();
        int32_t val = rand();

        to_insert->push_back(std::pair<int32_t, int32_t>(key, val));
        test->insert(key, val);
    }

    for (size_t i=0; i<n; i++) {
        int32_t key = to_insert->at(i).first;
        int32_t val = to_insert->at(i).second;

        int32_t testval = test->get(key);
        ck_assert_int_eq(val, testval);
    }

    delete to_insert;
    delete test;
}
END_TEST


START_TEST(duplicate_insert)
{
    auto test = new HashTable<int32_t, int32_t>(10);
    srand(0);
    for (int i=0; i<1000; i++) {
        int32_t key = rand();
        int32_t val = rand();

        test->insert(key, val);
    }

    int32_t res = test->insert(101, 50);
    ck_assert_int_eq(res, 50);

    res = test->insert(101, 75);
    ck_assert_int_eq(res, 50);

    delete test;
}


START_TEST(read_miss)
{
    auto test = new HashTable<int32_t, int32_t>(10);
    srand(0);
    for (int i=0; i<1000; i++) {
        int32_t key = rand();
        int32_t val = rand();

        test->insert(key, val);
    }

    bool error = false;
    try {
        test->get(45);
    } catch (KeyNotFoundException& e) {
        error = true;
    }

    ck_assert_int_eq(error, true);

    delete test;
}
END_TEST


START_TEST(remove_test)
{

    auto test = new HashTable<int32_t, int32_t>(10);
    auto to_insert = new std::vector<std::pair<int32_t, int32_t>>();
    srand(0);

    size_t n = 1000;
    for (size_t i=0; i<n; i++) {
        int32_t key = rand();
        int32_t val = rand();

        to_insert->push_back(std::pair<int32_t, int32_t>(key, val));
        test->insert(key, val);
    }


    int32_t key = 12345;
    int32_t val = 67583;

    int32_t res = test->insert(key, val);
    ck_assert_int_eq(res, val);

    res = test->get(key);
    ck_assert_int_eq(res, val);

    bool error = false;
    test->remove(key);

    try {
        res = test->get(key);
    } catch (KeyNotFoundException& e) {
        error = true;
    }

    ck_assert_int_eq(error, true);

    for (size_t i=0; i<n; i++) {
        int32_t key = to_insert->at(i).first;
        int32_t val = to_insert->at(i).second;

        int32_t testval = test->get(key);
        ck_assert_int_eq(val, testval);
    }

    delete test;
    delete to_insert;
}
END_TEST

START_TEST(remove_miss)
{
    auto test = new HashTable<int32_t, int32_t>(10);
    srand(0);
    for (int i=0; i<1000; i++) {
        int32_t key = rand();
        int32_t val = rand();

        test->insert(key, val);
    }

    bool error = false;
    try {
        test->remove(45);
    } catch (KeyNotFoundException& e) {
        error = true;
    }

    ck_assert_int_eq(error, true);

    delete test;
}
END_TEST


Suite *test_suite()
{
    Suite *suite = suite_create("HashTable Tests");

    // Test the basic functionality
    TCase *basic = tcase_create("basic");
    tcase_add_test(basic, create);
    tcase_add_test(basic, destroy);
    tcase_add_test(basic, insert);
    tcase_add_test(basic, duplicate_insert);
    tcase_add_test(basic, read_miss);
    tcase_add_test(basic, read_test);
    tcase_add_test(basic, remove_test);
    tcase_add_test(basic, remove_miss);

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
