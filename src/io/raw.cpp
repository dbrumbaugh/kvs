/*
 *
 */
#include "kvs.hpp"
#include "io/raw.hpp"
#include "io/exceptions.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

RawIOHandler::RawIOHandler(const char *filename)
{
    this->fd = open(filename, O_CREAT | O_RDWR);
    if (this->fd == -1) throw IOException();

    // Verify that this->fd refers to a regular file. If not, then it cannot
    // be seeked, and so pread/prwrite won't work. I'm sure that there are
    // other, non-regular, files that could be used here too, but for the moment
    // I'm going to restrict usage to them.
    struct stat statbuff;
    fstat(this->fd, &statbuff);

    if (!S_ISREG(statbuff.st_mode)) throw IOException();

}


RawIOHandler::~RawIOHandler()
{
    fsync(this->fd);
    close(this->fd);
}


fd_t RawIOHandler::get_fd()
{
    return this->fd;
}

int RawIOHandler::perform_io(byte* buffer, size_t size, off_t offset, op_t op)
{
    ssize_t total_progress = 0;
    ssize_t progress;

    do {
        if (op == op_t::READ)
            progress = pread(this->fd, buffer + total_progress,
                    size - total_progress, offset + total_progress);
        else if (op == op_t::WRITE)
            progress = pwrite(this->fd, buffer + total_progress,
                    size - total_progress, offset + total_progress);

        if (progress == -1) throw IOException();

        total_progress += progress;

    } while (total_progress != size);

    return (int) total_progress;
}


int RawIOHandler::read(byte* buffer, size_t size, off_t offset)
{
    if (offset + size > this->get_flen()) throw IOException();
    return this->perform_io(buffer, size, offset, op_t::READ);
}



int RawIOHandler::write(byte* buffer, size_t size, off_t offset)
{
    return this->perform_io(buffer, size, offset, op_t::WRITE);
}



off_t RawIOHandler::get_flen()
{
    off_t end = lseek(this->fd, 0, SEEK_END);
    if (end == -1) throw IOException();

    return end;
}
