#include <check.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include "io/buffer.hpp"
#include "io/block.hpp"

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


off_t WRITE_FILE_BLOCKS = 4;
int *WRITE_DATA_GT;
size_t WRITE_GT_LEN;
const char *write_file = "./tests/data/buffer/write_file.dat";

void setup_write()
{
    int write_fd = open(write_file, FFLAGS | O_TRUNC, 0644);
    WRITE_GT_LEN = WRITE_FILE_BLOCKS * Buffer::pagesize;
    size_t element_cnt = WRITE_GT_LEN / sizeof(int);

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

    close(write_fd);
}


void teardown_write()
{
    delete[] WRITE_DATA_GT;
}


START_TEST(constructor_test)
{
    using namespace Buffer::Test;
    using Buffer::create_manager;
    using Buffer::s_manager_ptr;
    using block::verify_valid_file_len;

    bool error = false;

    s_manager_ptr manager;

    try {
        manager = create_manager(write_file);
    } catch (exception& e) {
        error = true;
    }

    ck_assert_int_eq(error, false);
    ck_assert_int_eq(manager_get_max_page_count(manager), Buffer::default_pool_page_count);
    ck_assert_int_eq(manager_get_current_page_count(manager), 0);

    int fd = manager_get_fd(manager);
    int valid = fcntl(fd, F_GETFD);
    ck_assert_int_ne(valid, -1);
    ck_assert_int_eq(verify_valid_file_len(fd), true);
}
END_TEST


START_TEST(destructor_test)
{
    using namespace Buffer::Test;
    using Buffer::create_manager;
    using Buffer::s_manager_ptr;

    auto manager = create_manager(write_file);

    int fd = manager_get_fd(manager);

    manager.reset();

    // verify file is no longer valid
    int valid = fcntl(fd, F_GETFD);
    ck_assert_int_eq(valid, -1);
    ck_assert_int_eq(errno, EBADF);
}
END_TEST


START_TEST(pin_test)
{
    using namespace Buffer::Test;
    using Buffer::create_manager;

    size_t page_id = 0;
    auto manager = create_manager(write_file);

    auto page = manager->pin_page(page_id);
    Buffer::pmeta_t *meta = manager_get_meta(manager)->at(page_id);

    ck_assert_int_eq(meta->clock_ref, 1);
    ck_assert_int_eq(meta->pinned, 1);
    ck_assert_int_eq(meta->page_memory_offset, 0);
    ck_assert_int_eq(page->get_page_id(), page_id);

    ck_assert_mem_eq(page->get_page_data(), WRITE_DATA_GT, Buffer::pagesize);

}
END_TEST


START_TEST(clean_unpin_test)
{
    using namespace Buffer::Test;
    using Buffer::create_manager;

    size_t page_id = 0;
    auto manager = create_manager(write_file);

    {
        auto page = manager->pin_page(page_id);
    }

    // page should be unpinned at this point
    Buffer::pmeta_t *meta = manager_get_meta(manager)->at(page_id);
    ck_assert_int_eq(meta->clock_ref, 1);
    ck_assert_int_eq(meta->pinned, 0);

}
END_TEST


START_TEST(dirty_unpin_test)
{
    using namespace Buffer::Test;
    using Buffer::create_manager;
    using Buffer::pagesize;

    size_t page_id = 0;
    auto manager = create_manager(write_file);

    {
        auto page = manager->pin_page(0);
        memcpy(page->get_page_data(), ((byte *) WRITE_DATA_GT) + pagesize, pagesize);
        page->mark_modified();
    }

    // The page should be marked as modified
    Buffer::pmeta_t *meta = manager_get_meta(manager)->at(page_id);
    ck_assert_int_eq(meta->modified, 1);

    manager.reset();

    // on delete, the contents should be flushed to the file.
    byte* file_data = new byte[pagesize];
    int fd = open(write_file, O_RDONLY);

    size_t total_progress = 0;
    while (total_progress < pagesize) {
        ssize_t progress = pread(fd, file_data + total_progress, pagesize - total_progress, 0 + total_progress );
        if (progress == -1) abort();
        total_progress += (size_t) progress;
    }
    close(fd);

    ck_assert_mem_eq(file_data, ((byte *) WRITE_DATA_GT) + pagesize, pagesize);

    delete[] file_data;
}
END_TEST


Suite *unit_testing()
{
    Suite *unit = suite_create("Buffer IO Unit Testing");

    TCase *ctor = tcase_create("ctor");
    tcase_add_test(ctor, constructor_test);
    tcase_add_test(ctor, destructor_test);
    suite_add_tcase(unit, ctor);

    TCase *pin = tcase_create("pin");
    tcase_add_unchecked_fixture(pin, setup_write, teardown_write);
    tcase_add_test(pin, pin_test);
    tcase_add_test(pin, clean_unpin_test);
    tcase_add_test(pin, dirty_unpin_test);
    suite_add_tcase(unit, pin);

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
