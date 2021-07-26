/*
 *
 */

#include "io/buffer.hpp"
#include "io/block.hpp"
#include "unistd.h"
#include <fcntl.h>
#include <memory>
#include <unordered_map>

Buffer::Manager::Manager(const char *filename)
{
    this->backing_fd = open(filename, O_RDWR | O_CREAT, 0644);
    block::prepare_file(this->backing_fd);

    this->page_data = new std::unordered_map<size_t, pmeta_t>();
    this->buffer_data = new byte[default_pool_page_count * pagesize];
    this->clock = new std::queue<size_t>();
}


Buffer::Manager::Manager(const char*filename, size_t buffer_pool_page_count)
{
    this->backing_fd = open(filename, O_RDWR | O_CREAT, 0644);
    block::prepare_file(this->backing_fd);

    this->page_data = new std::unordered_map<size_t, pmeta_t>();
    this->buffer_data = new byte[buffer_pool_page_count * pagesize];
    this->clock = new std::queue<size_t>();
}


void Buffer::Manager::unpin_page(size_t page_id)
{
    pmeta_t page_data;
    try {
        page_data = this->page_data->at(page_id);
    } catch (std::out_of_range) {
        return;
    }

    if (page_data.pinned) {
        page_data.pinned--;
        this->unlock_page(page_id);
    }
    // TODO: Perhaps raise an error when trying to unpin a page that isn't
    // currently pinned?
}


void Buffer::Manager::flush_page(size_t page_id)
{
    pmeta_t page_data;

    try {
        page_data = this->page_data->at(page_id);
    } catch (std::out_of_range) {
        return;
    }

    if (page_data.modified) {
        block::write(this->backing_fd, page_id,
                this->buffer_data + page_data.page_memory_offset);

        fsync(this->backing_fd);
    }

}


void Buffer::Manager::load_page(size_t page_id, bool pin)
{
    bool not_present = false;
    try {
        this->page_data->at(page_id);
    } catch (std::out_of_range) {
        not_present = true;
    }

    if (not_present) {
        pmeta_t meta = pmeta_t {.page_id = page_id, .page_memory_offset = 0,
                                .pinned = 0, .modified = 0, .clock_ref = 0};
        if (this->has_space()) {
            // we'll fill from front to back. Once it's full, all new pages will
            // swap into the spot of other ones, and so we don't actually need
            // to explicitly track available page offsets within the buffer.
            meta.page_memory_offset = Buffer::pagesize * this->current_page_count;
        } else {
            size_t page_to_evict = find_page_to_evict();
            meta.page_memory_offset = page_data->at(page_to_evict).page_memory_offset;
            this->unload_page(page_to_evict);
        }

        block::read(this->backing_fd, page_id, this->buffer_data +
                meta.page_memory_offset);
        if (pin) meta.pinned++;
        this->page_data->insert(std::pair<size_t, pmeta_t> {page_id, meta});
        this->current_page_count++;
        this->clock->push(page_id);
    }
}


void Buffer::Manager::unload_page(size_t page_id)
{
    this->flush_page(page_id);
    this->page_data->erase(page_id);
    this->current_page_count--;
}


Buffer::u_page_ptr Buffer::Manager::pin_page(size_t page_id, bool lock)
{
    using Buffer::u_page_ptr;

    pmeta_t page_data;

    try {
        page_data = this->page_data->at(page_id);
        page_data.pinned++;
    } catch (std::out_of_range) {
        this->load_page(page_id, true);
        page_data = this->page_data->at(page_id);
    }

    if (lock) {
        this->lock_page(page_id);
    }

    page_data.clock_ref = 1;

    u_page_ptr page = std::make_unique<Page>(page_id, this,
            this->buffer_data + page_data.page_memory_offset);

    return page;
}


Buffer::Manager::~Manager()
{
    for (auto pg : *this->page_data) {
        this->unload_page(pg.second.page_id);
    }

    delete[] this->buffer_data;
    delete this->page_data;
}


bool Buffer::Manager::has_space()
{
    return this->current_page_count < this->max_page_count;
}


size_t Buffer::Manager::find_page_to_evict()
{
    size_t evict_page_id;
    bool found = false;

    while (!found) {
        size_t next = this->clock->front();
        this->clock->pop();

        pmeta_t meta = this->page_data->at(next);

        if (meta.pinned == 0) {
            if (meta.clock_ref == 0){
                evict_page_id = meta.page_id;
                found = true;
            } else {
                meta.clock_ref = 0;
                this->clock->push(next);
        }
    }
}

return evict_page_id;
}


void Buffer::Manager::lock_page(size_t page_id)
{
    //TODO: implement lock structure
    return;
}


void Buffer::Manager::unlock_page(size_t page_id)
{
    //TODO: implement lock structure
    return;
}
