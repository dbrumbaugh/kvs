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


#define TESTING

namespace Buffer {
    const size_t pagesize = 4096;
    const size_t default_pool_page_count = 10;

    typedef struct {
        size_t page_id;
        size_t page_memory_offset;
        int pinned;
        int modified;
        int clock_ref;
    } pmeta_t;

    class Page;
    class Manager;

    typedef std::unique_ptr<Page> u_page_ptr;
    typedef std::shared_ptr<Page> s_page_ptr;
    typedef std::shared_ptr<Manager> s_manager_ptr;

    s_manager_ptr create_manager(const char *filename,
            size_t buffer_pool_page_count=default_pool_page_count);

    namespace Test {
        byte *manager_get_data(s_manager_ptr man);
        std::unordered_map<size_t, pmeta_t*> *manager_get_meta(s_manager_ptr man);
        std::queue<size_t> *manager_get_clock(s_manager_ptr man);
        int manager_get_fd(s_manager_ptr man);
        size_t manager_get_max_page_count(s_manager_ptr man);
        size_t manager_get_current_page_count(s_manager_ptr man);
    }

    class Manager : public std::enable_shared_from_this<Manager>
    {
        friend Page;

#ifdef TESTING
        friend byte *Test::manager_get_data(s_manager_ptr man);
        friend std::unordered_map<size_t, pmeta_t*> *Test::manager_get_meta(s_manager_ptr man);
        friend std::queue<size_t> *Test::manager_get_clock(s_manager_ptr man);
        friend int Test::manager_get_fd(s_manager_ptr man);
        friend size_t Test::manager_get_max_page_count(s_manager_ptr man);
        friend size_t Test::manager_get_current_page_count(s_manager_ptr man);
#endif

        private:
            byte* buffer_data;
            std::unordered_map<size_t, pmeta_t*> *page_data;
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
            void unload_page(size_t page_id, bool erase_meta=true);
            size_t find_page_to_evict();

        public:
            Manager(const char *filename);
            Manager(const char*filename, size_t buffer_pool_page_count);

            u_page_ptr pin_page(size_t page_id, bool lock=false);

            ~Manager();

    };


    class Page {
        private:
            s_manager_ptr manager;
            bool pinned;
            const bool auto_unpin;

        public:
            Page(size_t page_id, s_manager_ptr man, byte *data,
                    bool auto_upin=true, bool pinned=true);
            const size_t id;
            byte* const data;
            void mark_modified();
            void unpin();

            ~Page();
        };
}
#endif
