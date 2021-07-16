/*
 *
 */
#ifndef iohandler
#define iohandler

#include "kvs.hpp"
#include <cstdlib>

#define BLOCKSIZE 100
off_t block_offset(size_t block);

class IOHandler
{
    public:
//        virtual byte* read(size_t block)=0;
 //       virtual void write(size_t block, byte *data)=0;
  //      virtual off_t offset(size_t block)=0;
   //     virtual off_t flen()=0;
        virtual size_t get_bs()=0;
        virtual fd_t get_fd()=0;
        virtual ~IOHandler(){};
};
#endif
