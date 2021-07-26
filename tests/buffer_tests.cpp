#include <check.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include "io/buffer.hpp"


// TODO: Figure out how to force pread/pwrite to return only a partial
// read/write to ensure that the handlers for that work. I suppose I could
// create dummy versions of the functions for testing.

using namespace std;

int FFLAGS = O_RDWR | O_CREAT;

int read_fd;
off_t READ_FILE_BLOCKS = 4;
int *READ_DATA_GT;

void setup_read()
{
    const char *read_file = "./tests/data/buffer/read_file.dat";
    read_fd = open(read_file, FFLAGS | O_TRUNC, 0644);
    size_t element_cnt = READ_FILE_BLOCKS * Buffer::pagesize / sizeof(int);

    srand(0);
    READ_DATA_GT = new int[element_cnt];
    for (size_t i=0; i<element_cnt;i++) {
        READ_DATA_GT[i] = rand();
    }

    size_t total_progress = 0;
    ssize_t progress = 0;
    while (total_progress < element_cnt*sizeof(int)) {
        progress = pwrite(read_fd, READ_DATA_GT, element_cnt * sizeof(int), 0);
        if (progress == -1) abort();
        total_progress += (size_t) progress;
    }
}


void teardown_read()
{
    delete[] READ_DATA_GT;
    close(read_fd);
}


int write_fd;
off_t WRITE_FILE_BLOCKS = 4;
int *WRITE_DATA_GT;
const char *write_file = "./tests/data/buffer/write_file.dat";

void setup_write()
{
    write_fd = open(write_file, FFLAGS | O_TRUNC, 0644);
    size_t element_cnt = WRITE_FILE_BLOCKS * Buffer::pagesize / sizeof(int);

    srand(5);
    WRITE_DATA_GT = new int[element_cnt];
    for (size_t i=0; i<element_cnt;i++) {
        WRITE_DATA_GT[i] = rand();
    }

    size_t total_progress = 0;
    ssize_t progress = 0;
    while (total_progress < element_cnt*sizeof(int)) {
        progress = pwrite(write_fd, WRITE_DATA_GT, element_cnt * sizeof(int), 0);
        if (progress == -1) abort();
        total_progress += (size_t) progress;
    }
}


void teardown_write()
{
    delete[] WRITE_DATA_GT;
    close(write_fd);
}


START_TEST(constructor_test)
{
    bool error = false;

    try {
        auto manager = Buffer::create_manager(write_file);
    } catch (exception& e) {
        error = true;
    }

    ck_assert_int_eq(error, false);
}
END_TEST


START_TEST(destructor_test)
{
    auto manager = new Buffer::Manager(write_file);
    delete manager;
}
END_TEST


START_TEST(pin_test)
{
    size_t page_id = 0;
    auto manager = Buffer::create_manager(write_file);

    auto page = manager->pin_page(page_id);

    ck_assert_int_eq(page->get_page_id(), page_id);
}
END_TEST


START_TEST(unpin_test)
{
    size_t page_id = 0;
    auto manager = Buffer::create_manager(write_file);

    {
        auto page = manager->pin_page(page_id);
    }

    // page should be unpinned at this point
}
END_TEST


/*
START_TEST(write_test)
{
    using namespace Buffer;
    size_t n = pagesize / sizeof(int);
    int *write_data = new int[n];

    srand(9);
    for (size_t i=0; i<n; i++) {
        write_data[i] = rand();
    }


    auto manager = new Buffer::Manager(write_file);
    auto page = manager->pin_page(0);


    try {

        write(write_fd, 2, (byte *) write_data);
    } catch (exception& e) {
        delete[] write_data;
        abort();
    }


    int *file_data = new int[n];
    size_t total_progress = 0;
    ssize_t progress = 0;

    while (total_progress < n*sizeof(int)) {
        progress = pread(write_fd, file_data, n * sizeof(int),
                2 * pagesize);
        if (progress == -1) {
            delete[] write_data;
            delete[] file_data;
            abort();
        }
        total_progress += (size_t) progress;
    }

    ck_assert_mem_eq(file_data, write_data, sizeof(int) * n);

    delete[] write_data;
    delete[] file_data;
}
END_TEST


START_TEST(read_test)
{
    using namespace Buffer;
    size_t n = BLOCKSIZE / sizeof(int);
    int *read_buffer = new int[n];

    int res;
    try {
        res = read(read_fd, 0, (byte *) read_buffer);
    } catch (exception& e) {
        delete[] read_buffer;
        abort();
    }
    ck_assert_int_eq(res, BLOCKSIZE);
    ck_assert_mem_eq(read_buffer, READ_DATA_GT, BLOCKSIZE);

    try {
        res = read(read_fd, 1, (byte *) read_buffer);
    } catch (exception& e) {
        delete[] read_buffer;
        abort();
    }
    ck_assert_int_eq(res, BLOCKSIZE);
    ck_assert_mem_eq(read_buffer, READ_DATA_GT + BLOCKSIZE/sizeof(int), BLOCKSIZE);

    try {
        res = read(read_fd, 2, (byte *) read_buffer);
    } catch (exception& e) {
        delete[] read_buffer;
        abort();
    }
    ck_assert_int_eq(res, BLOCKSIZE);
    ck_assert_mem_eq(read_buffer, READ_DATA_GT + 2*BLOCKSIZE/sizeof(int), BLOCKSIZE);

    delete[] read_buffer;
}
END_TEST


START_TEST(read_after_end)
{
    using namespace Buffer;
    size_t n = BLOCKSIZE / sizeof(int);
    int *read_buffer = new int[n]();

    int res;
    try {
        res = read(read_fd, READ_FILE_BLOCKS+1, (byte *) read_buffer);
    } catch (exception& e) {
        delete[] read_buffer;
        abort();
    }

    ck_assert_int_eq(res, 0);
    for (size_t i=0; i<n; i++) {
        ck_assert_int_eq(read_buffer[i], 0);
    }

    delete[] read_buffer;
}
END_TEST
*/


Suite *unit_testing()
{
    Suite *unit = suite_create("Buffer IO Unit Testing");

    TCase *ctor = tcase_create("ctor");
    tcase_add_test(ctor, constructor_test);
    tcase_add_test(ctor, destructor_test);
    suite_add_tcase(unit, ctor);

    TCase *pin = tcase_create("pin");
    tcase_add_test(pin, pin_test);
    tcase_add_test(pin, unpin_test);
    suite_add_tcase(unit, pin);


    /*
    // Testing read functionality
    TCase *read = tcase_create("read");
    tcase_add_unchecked_fixture(read, setup_read, teardown_read);
    tcase_add_test(read, read_test);
    tcase_add_test(read, read_after_end);
    suite_add_tcase(unit, read);

    // Testing write functionality
    TCase *write = tcase_create("write");
    tcase_add_unchecked_fixture(write, setup_write, teardown_write);
    tcase_add_test(write, write_test);
    suite_add_tcase(unit, write);

    // Testing append functionality
    TCase *append = tcase_create("append");
    // not a bug--the reuse of the write fixtures for append
    // testing is intended.
    tcase_add_unchecked_fixture(append, setup_write, teardown_write);
    tcase_add_test(append, append_test);
    suite_add_tcase(unit, append);
    */

    return unit;
}


Suite *stress_testing()
{
    // TODO: Add stress testing
    Suite *stress = suite_create("stress");

    return stress;

}


Suite *integration_testing()
{
    // TODO: Add stress testing
    Suite *integration = suite_create("integration");

    return integration;
}


int run_unit_tests()
{
    int failed = 0;
    Suite *unit = unit_testing();
    SRunner *unit_runner = srunner_create(unit);

    srunner_run_all(unit_runner, CK_VERBOSE);
    failed = srunner_ntests_failed(unit_runner);
    srunner_free(unit_runner);

    return failed;
}


int main()
{
    int unit_failed = run_unit_tests();

    return (unit_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
