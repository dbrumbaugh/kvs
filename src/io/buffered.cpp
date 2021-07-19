#include "io/buffered.hpp"
#include <unordered_map>


BufferedIOHandler::BufferedIOHandler(IOHandler* iodev)
{
    this->buffer_cnt = 0;
    this->buffer_pool = new std::unordered_map<int, byte*>();
    this->iodev = iodev;
}


BufferedIOHandler::~BufferedIOHandler()
{
    //TODO: flush the buffers first
    buffer_pool->clear();
    delete buffer_pool;
}


int BufferedIOHandler::read(byte* buffer, size_t size, off_t offset)
{

    return 0;
}


int BufferedIOHandler::write(byte* buffer, size_t size, off_t offset)
{
    return 0;
}


off_t BufferedIOHandler::get_flen()
{
    return this->iodev->get_flen();
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
    //TODO: Read page from underlying IO device.
    if (this->buffer_pool->find(buffno) == this->buffer_pool->end()) {
        this->buffer_pool->insert({buffno, new byte[buffer_size]()});
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
        //TODO: Read page from underlying IO device.
    }

    return buffer;
}
