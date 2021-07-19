/*
 *
 */
#ifndef iohandler
#define iohandler

#include "kvs.hpp"
#include <cstdlib>

enum class op_t {
    READ,
    WRITE
};

class IOHandler
{
    public:
        virtual int read(byte* buffer, size_t size, off_t offset)=0;
        virtual int write(byte* buffer, size_t size, off_t offset)=0;
        virtual off_t get_flen()=0;
        virtual fd_t get_fd()=0;
        virtual ~IOHandler(){};
};
#endif
