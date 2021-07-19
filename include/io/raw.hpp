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
        int perform_io(byte* buffer, size_t size, off_t offset, op_t op);

    public:
        RawIOHandler(const char *filename);
        ~RawIOHandler();
        int read(byte* buffer, size_t size, off_t offset) override;
        int write(byte* buffer, size_t size, off_t offset) override;
        off_t get_flen() override;
        int get_fd() override;
};
#endif
