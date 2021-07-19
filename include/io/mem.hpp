
/*
 *
 */
#ifndef memio
#define memio

#include "kvs.hpp"
#include "io/iohandler.hpp"
#include <cstdio>

class MemIOHandler: public IOHandler
{
    private:
        bool resize;
        byte *buffer;
        size_t size;
        off_t len;

    public:
        MemIOHandler(size_t initial_size);
        MemIOHandler(size_t initial_size, bool resize);
        ~MemIOHandler();
        int read(byte* buffer, ssize_t size, off_t offset) override;
        int write(byte* buffer, ssize_t size, off_t offset) override;
        off_t get_flen() override;
        int get_fd() override;
};
#endif
