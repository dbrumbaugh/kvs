#include <check.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include "io/block.hpp"


// TODO: Figure out how to force pread/pwrite to return only a partial
// read/write to ensure that the handlers for that work. I suppose I could
// create dummy versions of the functions for testing.

using namespace std;

int FFLAGS = O_RDWR | O_CREAT;

int len_fd_valid_size;
int len_fd_invalid_size;

off_t LEN_VAL_BLOCKS = 5;
off_t LEN_INVAL_BLOCKS = 2;
off_t LEN_VAL_BYTES = block::BLOCKSIZE * LEN_VAL_BLOCKS;
off_t LEN_INVAL_BYTES = block::BLOCKSIZE * LEN_INVAL_BLOCKS + 125;

void setup_len()
{
    const char *valid = "./tests/data/block/valid.dat";
    const char *invalid = "./tests/data/block/invalid.dat";


    len_fd_valid_size = open(valid, FFLAGS | O_TRUNC, 0644);
    len_fd_invalid_size = open(invalid, FFLAGS | O_TRUNC, 0644);


    // now set them up to the appropriate length for testing
    ftruncate(len_fd_valid_size, LEN_VAL_BYTES);
    ftruncate(len_fd_invalid_size, LEN_INVAL_BYTES);
}


void teardown_len()
{
    close(len_fd_valid_size);
    close(len_fd_invalid_size);
}

int trunc_grow_fd;
off_t TRUNC_GROW_BLOCKS = 4;
off_t TRUNC_GROW_BYTES = TRUNC_GROW_BLOCKS * block::BLOCKSIZE;

int trunc_shrink_fd;
off_t TRUNC_SHRINK_BLOCKS = 4;
off_t TRUNC_SHRINK_BYTES = TRUNC_SHRINK_BLOCKS * block::BLOCKSIZE;

int trunc_prep_fd;
off_t TRUNC_PREP_BLOCKS = 2;
off_t TRUNC_PREP_BYTES = TRUNC_PREP_BLOCKS * block::BLOCKSIZE + 300;

void setup_trunc()
{
    const char *grow_file = "./tests/data/block/trunc_grow.dat";
    const char *shrink_file = "./tests/data/block/trunc_shrink.dat";
    const char *prep_file = "./tests/data/block/trunc_prep.dat";

    trunc_grow_fd = open(grow_file, FFLAGS | O_TRUNC, 0644);
    ftruncate(trunc_grow_fd, TRUNC_GROW_BYTES);

    trunc_shrink_fd = open(shrink_file, FFLAGS | O_TRUNC, 0644);
    ftruncate(trunc_shrink_fd, TRUNC_SHRINK_BYTES);

    trunc_prep_fd = open(prep_file, FFLAGS | O_TRUNC, 0644);
    ftruncate(trunc_prep_fd, TRUNC_PREP_BYTES);
}


void teardown_trunc()
{
    close(trunc_grow_fd);
    close(trunc_shrink_fd);
    close(trunc_prep_fd);
}


int read_fd;
off_t READ_FILE_BLOCKS = 4;
int *READ_DATA_GT;

void setup_read()
{
    const char *read_file = "./tests/data/block/read_file.dat";
    read_fd = open(read_file, FFLAGS | O_TRUNC, 0644);
    size_t element_cnt = READ_FILE_BLOCKS * block::BLOCKSIZE / sizeof(int);

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

void setup_write()
{
    const char *write_file = "./tests/data/block/write_file.dat";
    write_fd = open(write_file, FFLAGS | O_TRUNC, 0644);
    size_t element_cnt = WRITE_FILE_BLOCKS * block::BLOCKSIZE / sizeof(int);

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


START_TEST(len_bytes_test)
{
    using namespace block;

    off_t valid_len_bytes = _file_len_bytes(len_fd_valid_size);
    ck_assert_int_eq(valid_len_bytes, LEN_VAL_BYTES);

    off_t invalid_len_bytes = _file_len_bytes(len_fd_invalid_size);
    ck_assert_int_eq(invalid_len_bytes, LEN_INVAL_BYTES);

}
END_TEST


START_TEST(len_blocks_test)
{
    using namespace block;

    off_t valid_len_blocks = file_len_blocks(len_fd_valid_size);
    ck_assert_int_eq(valid_len_blocks, LEN_VAL_BLOCKS);

    off_t invalid_len_blocks = file_len_blocks(len_fd_invalid_size);
    ck_assert_int_eq(invalid_len_blocks, LEN_INVAL_BLOCKS);
}
END_TEST


START_TEST(verify_len_test)
{
    using namespace block;
    bool valid = block::verify_valid_file_len(len_fd_valid_size);
    ck_assert_int_eq(valid, true);

    bool invalid = block::verify_valid_file_len(len_fd_invalid_size);
    ck_assert_int_eq(invalid, false);
}
END_TEST


START_TEST(truncate_grow_test)
{
    using namespace block;
    off_t new_block_len = TRUNC_GROW_BLOCKS + 3;

    int res = truncate_to_block_size(trunc_grow_fd, new_block_len);
    ck_assert_int_eq(res, 0);
    ck_assert_int_eq(file_len_blocks(trunc_grow_fd), new_block_len);
    ck_assert_int_eq(_file_len_bytes(trunc_grow_fd), new_block_len * BLOCKSIZE);
}
END_TEST


START_TEST(truncate_shrink_test)
{
    using namespace block;
    off_t new_block_len = TRUNC_SHRINK_BLOCKS - 2;

    int res = truncate_to_block_size(trunc_shrink_fd, new_block_len);
    ck_assert_int_eq(res, 0);
    ck_assert_int_eq(file_len_blocks(trunc_shrink_fd), new_block_len);
    ck_assert_int_eq(_file_len_bytes(trunc_shrink_fd), new_block_len * BLOCKSIZE);
}
END_TEST


START_TEST(prepare_test)
{
    using namespace block;
    prepare_file(trunc_prep_fd);
    ck_assert_int_eq(file_len_blocks(trunc_prep_fd), TRUNC_PREP_BLOCKS + 1);
    ck_assert_int_eq(_file_len_bytes(trunc_prep_fd), (TRUNC_PREP_BLOCKS + 1) * BLOCKSIZE);
}
END_TEST


START_TEST(write_test)
{
    using namespace block;
    size_t n = BLOCKSIZE / sizeof(int);
    int *write_data = new int[n];

    srand(9);
    for (size_t i=0; i<n; i++) {
        write_data[i] = rand();
    }

    off_t presize;
    try {
        presize = file_len_blocks(write_fd);
    } catch (exception& e) {
        delete[] write_data;
        abort();
    }

    try {
        write(write_fd, 2, (byte *) write_data);
    } catch (exception& e) {
        delete[] write_data;
        abort();
    }

    try {
        ck_assert_int_eq(presize, file_len_blocks(write_fd));
        ck_assert_int_eq(verify_valid_file_len(write_fd), true);
    } catch (exception& e) {
        delete[] write_data;
        abort();
    }

    int *file_data = new int[n];
    size_t total_progress = 0;
    ssize_t progress = 0;

    while (total_progress < n*sizeof(int)) {
        progress = pread(write_fd, file_data, n * sizeof(int),
                2 * BLOCKSIZE);
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


START_TEST(append_test)
{
    using namespace block;
    size_t n = BLOCKSIZE / sizeof(int);
    int *append_data = new int[n];

    srand(9);
    for (size_t i=0; i<n; i++) {
        append_data[i] = rand();
    }

    off_t presize;
    try {
        presize = file_len_blocks(write_fd);
    } catch (exception& e) {
        delete[] append_data;
        abort();
    }

    try {
        append(write_fd, (byte *) append_data);
    } catch (exception& e) {
        delete[] append_data;
        abort();
    }

    try {
        ck_assert_int_eq(presize+1, file_len_blocks(write_fd));
        ck_assert_int_eq(verify_valid_file_len(write_fd), true);
    } catch (exception& e) {
        delete[] append_data;
        abort();
    }

    int *file_data = new int[n]();
    size_t total_progress = 0;
    ssize_t progress = 0;

    while (total_progress < n*sizeof(int)) {
        progress = pread(write_fd, file_data, n * sizeof(int),
                presize * BLOCKSIZE);
        if (progress == -1) {
            delete[] append_data;
            delete[] file_data;
            abort();
        }

        total_progress += (size_t) progress;
    }

    ck_assert_mem_eq(file_data, append_data, sizeof(int) * n);
    delete[] append_data;
    delete[] file_data;
}
END_TEST


START_TEST(read_test)
{
    using namespace block;
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
    using namespace block;
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


Suite *unit_testing()
{
    Suite *unit = suite_create("Block IO Unit Testing");


    // Testing length measurement functions
    TCase *len = tcase_create("length");
    tcase_add_unchecked_fixture(len, setup_len, teardown_len);
    tcase_add_test(len, len_bytes_test);
    tcase_add_test(len, len_blocks_test);
    tcase_add_test(len, verify_len_test);

    suite_add_tcase(unit, len);

    // Testing truncation functions
    TCase *trunc = tcase_create("truncate");
    tcase_add_unchecked_fixture(trunc, setup_trunc, teardown_trunc);
    tcase_add_test(trunc, truncate_grow_test);
    tcase_add_test(trunc, truncate_shrink_test);
    tcase_add_test(trunc, prepare_test);
    suite_add_tcase(unit, trunc);

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
