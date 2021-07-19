/*
 *
 */

#include "io/mem.hpp"
#include "io/exceptions.hpp"
#include <cstdlib>
#include <cstring>

MemIOHandler::MemIOHandler(size_t initial_size)
{
    this->buffer = new byte[initial_size];
    this->resize = false;
    this->size = initial_size;
    this->len = 0;

    if (!this->buffer) throw IOException();
}


MemIOHandler::MemIOHandler(size_t initial_size, bool resize)
{

    this->buffer = new byte[initial_size];
    this->resize = resize;
    this->size = initial_size;
    this->len = 0;

    if (!this->buffer) throw IOException();
}


MemIOHandler::~MemIOHandler()
{
    free(this->buffer);
}


int MemIOHandler::read(byte *buffer, ssize_t size, off_t offset)
{
    size_t read_size = size;
    if (offset + size >= this->size) {
        read_size = this->size - offset;
    }

    memmove(buffer, this->buffer + offset, read_size);

    return read_size;
}


int MemIOHandler::write(byte *buffer, ssize_t size, off_t offset)
{
    size_t write_size = size;
    // Ensure that all the data can fit in the buffer
    if (offset + size >= this->size) {
        // TODO: If I end up implementing dynamic resizing, that ought to
        // happen right here.
        write_size = this->size - offset;
    }

    // technically, it's hard to imagine a situation where this->buffer
    // and buffer would overlap. But better safe than sorry, I suppose.
    memmove(this->buffer + offset, buffer, write_size);

    this->len += (offset + write_size);

    return write_size;
}


int MemIOHandler::get_fd()
{
    return 0;
}


off_t MemIOHandler::get_flen()
{
    return this->len;
}
