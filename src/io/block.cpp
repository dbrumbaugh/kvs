/*
 *
 */

#include "io/block.hpp"
#include "unistd.h"
#include "io/exceptions.hpp"
#include "kvs.hpp"

off_t block::_file_len_bytes(int fd)
{
    off_t end = lseek(fd, 0, SEEK_END);
    if (end == -1) throw IOException();

    return end;
}


void block::prepare_file(int fd)
{
    bool valid = block::verify_valid_file_len(fd);
    if (valid) return;

    off_t file_len_blocks = block::file_len_blocks(fd);
    block::truncate_to_block_size(fd, file_len_blocks + 1);
}


bool block::verify_valid_file_len(int fd)
{
    off_t file_len_bytes = block::_file_len_bytes(fd);

    return !(file_len_bytes % block::BLOCKSIZE);
}


off_t block::file_len_blocks(int fd)
{
    off_t file_len_bytes = block::_file_len_bytes(fd);
    off_t file_len_blocks = file_len_bytes / block::BLOCKSIZE;

    return file_len_blocks;
}


off_t block::offset(off_t block_no)
{
    return block_no * block::BLOCKSIZE;
}


int block::write(int fd, off_t block_no, const byte *data)
{
    off_t offset = block::offset(block_no);
    size_t total_progress = 0;
    ssize_t progress = 0;

    while (total_progress < block::BLOCKSIZE) {
        progress = pwrite(fd, data + total_progress, block::BLOCKSIZE - total_progress,
                offset + total_progress);

        if (progress == -1) throw IOException();
        total_progress += (size_t) progress;
    }

    return (int) total_progress;
}


int block::read(int fd, off_t block_no, byte *data)
{
    if (block_no >= file_len_blocks(fd)) return 0;

    off_t offset = block::offset(block_no);
    size_t total_progress = 0;
    ssize_t progress = 0;

    while (total_progress < block::BLOCKSIZE) {
        progress = pread(fd, data + total_progress, block::BLOCKSIZE - total_progress,
                offset + total_progress);

        if (progress == -1) throw IOException();
        total_progress += (size_t) progress;
    }

    return (int) total_progress;
}


int block::append(int fd, byte *data)
{
    off_t new_block_no = block::file_len_blocks(fd);
    int written = block::write(fd, new_block_no, data);

    return written;
}


int block::truncate_to_block_size(int fd, off_t block_no)
{
    off_t current_file_len_block = block::file_len_blocks(fd);
    off_t truncate_offset = block::offset(block_no);

    int res = ftruncate(fd, truncate_offset);

    if (res == -1) {
        perror("Error: ");
        throw IOException();
    }

    return 0;
}
