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
}


Buffer::Manager::Manager(const char*filename, size_t buffer_pool_page_count)
{
    this->backing_fd = open(filename, O_RDWR | O_CREAT, 0644);
    block::prepare_file(this->backing_fd);

    this->page_data = new std::unordered_map<size_t, pmeta_t>();
    this->buffer_data = new byte[buffer_pool_page_count * pagesize];
}


void Buffer::Manager::unpin_page(size_t page_id)
{
    pmeta_t page_data;
    try {
        page_data = this->page_data->at(page_id);
    } catch (std::out_of_range) {
        return;
    }

    if (page_data.pinned) page_data.pinned--;
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


void Buffer::Manager::load_page(size_t page_id)
{
    bool not_present = false;
    try {
        this->page_data->at(page_id);
    } catch (std::out_of_range) {
        not_present = true;
    }

    if (not_present) {
        //TODO: Create meta record, evict page if needed, copy data into buffer
    }
}


void Buffer::Manager::unload_page(size_t page_id)
{
    this->flush_page(page_id);
    this->page_data->erase(page_id);
}


Buffer::u_page_ptr Buffer::Manager::pin_page(size_t page_id)
{
    using Buffer::u_page_ptr;

    pmeta_t page_data;

    try {
        page_data = this->page_data->at(page_id);
    } catch (std::out_of_range) {
        return nullptr;
    }

    page_data.pinned++;

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
