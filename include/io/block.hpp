/*
 *
 *
 */
#ifndef blockio
#define blockio

#include <cstdlib>
#include "kvs.hpp"

namespace block
{
    const size_t BLOCKSIZE = 4096;
    off_t _file_len_bytes(int fd);
    off_t file_len_blocks(int fd);
    off_t offset(off_t block_no);

    bool verify_valid_file_len(int fd);
    void prepare_file(int fd);

    int write(int fd, off_t block_no, const byte *data);
    int read(int fd, off_t block_no, byte *data);
    int append(int fd, byte *data);
    int truncate_to_block_size(int fd, off_t block_no);
}



#endif
