/*
 *
 */
#ifndef buffman
#define buffman

#include <cstdlib>
#include <memory>
#include <unordered_map>
#include "kvs.hpp"

namespace Buffer {
    const size_t pagesize = 4098;
    const size_t default_pool_page_count = 10;

    typedef struct {
        size_t page_id;
        size_t page_memory_offset;
        int pinned;
        int modified;
        int clock_hand;
    } pmeta_t;

    class Page;
    typedef std::unique_ptr<Page> u_page_ptr;
    typedef std::shared_ptr<Page> s_page_ptr;

    class Manager
    {
        friend Page;

        private:
            byte* buffer_data;
            std::unordered_map<size_t, pmeta_t> *page_data;
            int backing_fd;

            void unpin_page(size_t page_id);
            void flush_page(size_t page_id);
            void load_page(size_t page_id);
            void unload_page(size_t page_id);

        public:
            Manager(const char *filename);
            Manager(const char*filename, size_t buffer_pool_page_count);

            u_page_ptr pin_page(size_t page_id);

            ~Manager();

    };


    class Page {
        private:
            size_t page_id;
            Manager *manager;
            byte *data;

        public:
            Page(size_t page_id, Manager *man, byte *data);
            size_t get_page_id();
            byte *get_page_data();
            void mark_modified();

            ~Page() {
                manager->unpin_page(page_id);
            }
    };
}
#endif
