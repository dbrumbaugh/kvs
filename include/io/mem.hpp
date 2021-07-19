
/*
 *
 */
#ifndef memio
#define memio

#include "kvs.hpp"
#include "io/iohandler.hpp"
#include <unordered_map>
#include <cstdio>

class MemIOHandler: public IOHandler
{
    private:
        std::unordered_map<int, byte*> *buffer_pool;
        size_t buffer_size;
        off_t len;
        void new_buffer();
        void new_buffer(size_t buffno);
        size_t buffer_num(off_t offset);
        byte *get_buffer(size_t buffno, bool create);
        size_t buffer_cnt;
        byte *hole;

    public:
        MemIOHandler(size_t initial_size);
        ~MemIOHandler();
        int read(byte* buffer, size_t size, off_t offset) override;
        int write(byte* buffer, size_t size, off_t offset) override;
        off_t get_flen() override;
        int get_fd() override;
};
#endif
