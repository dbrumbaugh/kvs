/*
 *
 */

#include "io/mem.hpp"
#include "io/exceptions.hpp"
#include "kvs.hpp"
#include <cstdlib>
#include <cstring>
#include <unordered_map>

MemIOHandler::MemIOHandler(size_t initial_size)
{
    this->buffer_pool = new std::unordered_map<int, byte*>();
    this->buffer_size = initial_size;
    this->len = 0;
    this->buffer_cnt = 0;
    this->hole = new byte[buffer_size]();

    this->new_buffer();
}


MemIOHandler::~MemIOHandler()
{
    buffer_pool->clear();
    delete buffer_pool;
}


void MemIOHandler::new_buffer()
{
    this->buffer_pool->insert({this->buffer_cnt, new byte[buffer_size]()});
    this->buffer_cnt++;
}


void MemIOHandler::new_buffer(size_t buffno)
{
    if (this->buffer_pool->find(buffno) == this->buffer_pool->end()) {
        this->buffer_pool->insert({buffno, new byte[buffer_size]()});
        this->buffer_cnt++;
    }
}


size_t MemIOHandler::buffer_num(off_t offset)
{
    return (size_t) (offset / this->buffer_size);
}


byte *MemIOHandler::get_buffer(size_t buffno, bool create)
{
    byte *buffer;

    try {
        buffer = this->buffer_pool->at(buffno);
    } catch (std::out_of_range& except) {
        if (create) {
        new_buffer(buffno);
        buffer = this->buffer_pool->at(buffno);
        } else {
            buffer = this->hole;
        }
    }

    return buffer;
}


int MemIOHandler::read(byte *buffer, size_t size, off_t offset)
{
    int cur_buffno = buffer_num(offset);
    off_t buff_offset = offset - (cur_buffno * this->buffer_size);
    byte *cur_buff = this->get_buffer(cur_buffno, false);
    off_t read_offset = 0;
    size_t remaining = size;

    do {
        size_t tomove = std::min(this->buffer_size - buff_offset, remaining);
        memmove(buffer + read_offset, cur_buff + buff_offset, tomove);
        buff_offset = 0;
        read_offset += tomove;
        remaining -= tomove;
        if (remaining) cur_buff = this->get_buffer(++cur_buffno, false);
    } while (remaining);

    return size;
}


int MemIOHandler::write(byte *buffer, size_t size, off_t offset)
{
    int cur_buffno = buffer_num(offset);
    off_t buff_offset = offset - (cur_buffno * this->buffer_size);
    byte *cur_buff = this->get_buffer(cur_buffno, true);
    off_t write_offset = 0;
    size_t remaining = size;

    do {
        size_t tomove = std::min(this->buffer_size - buff_offset, remaining);
        memmove(cur_buff + buff_offset, buffer + write_offset, tomove);
        buff_offset = 0;
        write_offset += tomove;
        remaining -= tomove;
        if (remaining) cur_buff = this->get_buffer(++cur_buffno, true);
    } while (remaining);

    if (offset + size > (size_t) this->len) {
        this->len = offset + size;
    }

    return size;
}


int MemIOHandler::get_fd()
{
    return 0;
}


off_t MemIOHandler::get_flen()
{
    return this->len;
}


void MemIOHandler::dump(size_t line_size)
{
    for (auto& buff: *this->buffer_pool){
        fprintf(stderr, "%d------\n", buff.first);
        for (size_t i=0; i<this->buffer_size; i++) {
            fprintf(stderr, "%02hhx ", buff.second[i]);
            if ((i+1) % line_size == 0) fprintf(stderr, "\n");
        }
    }

}
