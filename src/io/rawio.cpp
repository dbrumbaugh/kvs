/*
 *
 */
#include "kvs.hpp"
#include "io/rawio.hpp"
#include "io/exceptions.hpp"
#include <unistd.h>
#include <fcntl.h>

RawIOHandler::RawIOHandler(const char *filename, size_t block_size)
{
    bs = block_size;
    fd = open(filename, O_APPEND | O_CREAT | O_RDWR);
    if (fd == -1) throw IOException();
}


RawIOHandler::~RawIOHandler()
{
    close(fd);
}


size_t RawIOHandler::get_bs()
{
    return bs;
}


fd_t RawIOHandler::get_fd()
{
    return fd;
}
