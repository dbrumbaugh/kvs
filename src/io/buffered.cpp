#include "io/buffered.hpp"
#include "io/exceptions.hpp"
#include <unordered_map>
#include <cstdlib>
#include <cstring>


BufferedIOHandler::BufferedIOHandler(IOHandler* iodev, size_t pool_size)
{
    this->buffer_cnt = 0;
    this->len = 0;
    this->buffer_pool = new std::unordered_map<int, byte*>();
    this->iodev = iodev;
    this->buffer_max = pool_size;
    this->buffer_size = PAGESIZE;
}


BufferedIOHandler::~BufferedIOHandler()
{
    for (auto x: *buffer_pool) {
        this->evict_buffer(x.first, true);
    }

    buffer_pool->clear();
    delete buffer_pool;
    delete this->iodev;
}


int BufferedIOHandler::read(byte* buffer, size_t size, off_t offset)
{
    int cur_buffno = buffer_num(offset);
    off_t buff_offset = offset - (cur_buffno * this->buffer_size);
    byte *cur_buff = this->get_buffer(cur_buffno);
    off_t read_offset = 0;
    size_t remaining = size;

    do {
        size_t tomove = std::min(this->buffer_size - buff_offset, remaining);
        memmove(buffer + read_offset, cur_buff + buff_offset, tomove);
        buff_offset = 0;
        read_offset += tomove;
        remaining -= tomove;
        if (remaining) cur_buff = this->get_buffer(++cur_buffno);
    } while (remaining);

    return size;
}


int BufferedIOHandler::write(byte* buffer, size_t size, off_t offset)
{
    int cur_buffno = buffer_num(offset);
    off_t buff_offset = offset - (cur_buffno * this->buffer_size);
    byte *cur_buff = this->get_buffer(cur_buffno);
    off_t write_offset = 0;
    size_t remaining = size;

    do {
        size_t tomove = std::min(this->buffer_size - buff_offset, remaining);
        memmove(cur_buff + buff_offset, buffer + write_offset, tomove);
        buff_offset = 0;
        write_offset += tomove;
        remaining -= tomove;
        if (remaining) cur_buff = this->get_buffer(++cur_buffno);
    } while (remaining);

    if (offset + size > (size_t) this->len) {
        this->len = offset + size;
    }

    return size;
}


off_t BufferedIOHandler::get_flen()
{
    return std::max(this->iodev->get_flen(), this->len);
}


int BufferedIOHandler::get_fd()
{
    return this->iodev->get_fd();
}



void BufferedIOHandler::new_buffer()
{
    //TODO: Read page from underlying IO device.
    this->buffer_pool->insert({this->buffer_cnt, new byte[buffer_size]()});
    this->buffer_cnt++;
}


void BufferedIOHandler::new_buffer(size_t buffno)
{
    if (this->buffer_pool->find(buffno) == this->buffer_pool->end()) {
        // TODO: Find a buffer to evict

        byte *data = new byte[buffer_size]();
        off_t boff = this->buffer_off(buffno);

        if (boff < this->iodev->get_flen()) {
            this->iodev->read(data, this->buffer_size, boff);
        }

        this->buffer_pool->insert({buffno, data});
        this->buffer_cnt++;
    }
}


size_t BufferedIOHandler::buffer_num(off_t offset)
{
    return (size_t) (offset / this->buffer_size);
}


byte *BufferedIOHandler::get_buffer(size_t buffno)
{
    byte *buffer;

    try {
        buffer = this->buffer_pool->at(buffno);
    } catch (std::out_of_range& except) {
        new_buffer(buffno);
        buffer = this->buffer_pool->at(buffno);
    }

    return buffer;
}


void BufferedIOHandler::flush_buffer(size_t buffno)
{
    byte *buff = nullptr;
    try {
        buff = this->buffer_pool->at(buffno);
    } catch (std::out_of_range& except) {
        // attempt to flush a page that isn't in memory. Just silently return.
        // I may switch this over to raising an exception.
        return;
    }

    int written = this->iodev->write(buff, this->buffer_size, buffer_off(buffno));
    if (written != PAGESIZE)
        throw IOException();
}


void BufferedIOHandler::evict_buffer(size_t buffno, bool override_pins=false)
{
    //TODO: when I implement pins, I'll need to verify that the buffer
    //      isn't pinned before doing any of this.
    bool buff_pinned = false;
    if (!override_pins || !buff_pinned) {
        this->flush_buffer(buffno);
        this->buffer_pool->erase(buffno);
        this->buffer_cnt--;
    }
}


off_t BufferedIOHandler::buffer_off(size_t buffno)
{
    return (off_t) buffno * this->buffer_size;
}
