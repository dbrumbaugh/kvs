/*
 *
 */
#ifndef rawio
#define rawio

#include "kvs.hpp"
#include "io/iohandler.hpp"
#include <cstdio>

class RawIOHandler: public IOHandler
{
    private:
        fd_t fd;
        size_t bs;

    public:
        RawIOHandler(const char *filename, size_t block_size);
        ~RawIOHandler();
        size_t get_bs() override;
        int get_fd() override;
};
#endif
