/*
 *
 */
#ifndef buffman
#define buffman

#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <queue>
#include "kvs.hpp"

namespace Buffer {
    const size_t pagesize = 4098;
    const size_t default_pool_page_count = 10;

    typedef struct {
        size_t page_id;
        size_t page_memory_offset;
        int pinned;
        int modified;
        int clock_ref;
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
            std::queue<size_t> *clock; // 'twould more efficient to just use a
                                      // circular array, but I want to play
                                      // around with the standard library a
                                      // bit.
            int backing_fd;
            size_t max_page_count;
            size_t current_page_count;

            void lock_page(size_t page_id);
            void unlock_page(size_t page_id);
            void unpin_page(size_t page_id);
            void flush_page(size_t page_id);
            void load_page(size_t page_id, bool pin);
            void unload_page(size_t page_id);
            size_t find_page_to_evict();
            bool has_space();

        public:
            Manager(const char *filename);
            Manager(const char*filename, size_t buffer_pool_page_count);

            u_page_ptr pin_page(size_t page_id, bool lock=false);

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
