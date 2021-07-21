/*
 *
 */

#include "io/mem.hpp"
#include "io/raw.hpp"
#include "kvs.hpp"
#include <memory>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>

class KeyNotFoundException: public std::runtime_error
{
    public:
        KeyNotFoundException() : runtime_error("Requested Key not found in Table") {}
};

template <typename TKey, typename TValue>
class HashTable
{
    private:
        /*
         * Definitions for calculating how many elements will fit in a
         * cacheline, or how many cachelines are needed per element. The basic
         * object we'll use here is a CACHELINE sized object, with the final
         * sizeof(off_t) bits reserved for use as the file offset containing
         * the next element in the chain.
         *
         * TODO: Adjust this to ensure proper stack alignment of the elements.
         * TODO: This setup currently wastes 8 bits at the end of every
         *       cachline. even if the KVP spans several lines, 8 bits are
         *       still set aside at the end of each one to hold an offset, and
         *       then not used.
         * TODO: Perhaps I could store a portion of the KVP's hash, or something
         *       similar, as part of this too. It would be less space efficient,
         *       but it would allow storing {0, 0} and similar. As it stands, a
         *       span of element_sz 0 bits will not be recognized as an element.
         * TODO: I'm going to consider a trailing sizeof(off_t) number of 0 bits
         *       to indicate the end of a chain. This should allow me to delete
         *       without a tombstone or anything (so long as I manage the offset
         *       for the surrounding chain components if I delete a full bucket)
         *       However, it also means that the bucket at offset 0 in the file
         *       can never be used as any link in a chain aside from the first.
         *       Perhaps a nonissue, but it could come up.
         */
        static constexpr size_t const element_sz = sizeof(TKey) + sizeof(TValue);
        static constexpr size_t const bucket_sz = 1 + (element_sz) / 
                                                    (REDUCED_CACHELINE);
        static constexpr size_t const bucket_bytes = bucket_sz * CACHELINE;
        static constexpr size_t const bucket_data_bytes = bucket_bytes - sizeof(off_t);
        static constexpr size_t const elements_per_bucket = 
                                             REDUCED_CACHELINE / element_sz;


        //std::unique_ptr<IOHandler> storage;
        IOHandler *storage;
        size_t bucket_cnt;

        /*
         * Use std::hash to calculate the hash of the key, then force it into
         * range of the bucket count. I'll play around with replacing the %
         * operator with shifting later--that can be a little finicky and I'd
         * rather keep it simple 'till it is working.
         *
         * Also, I need to figure out a good way of asserting at compile time
         * that std::hash is defined for a given TKey. Perhaps look a little
         * more closely at
         * https://stackoverflow.com/questions/12753997/check-if-type-is-hashable
         *
         * I suppose I could always fall back on function pointers, but that
         * feels inelegant. 
         */


        off_t inline bucket_offset(size_t bucket_no)
        {
            return (off_t) bucket_no * bucket_bytes;
        }


        off_t inline get_bucket(TKey key)
        {
            return bucket_offset(hash(key));
        }


        off_t inline key_offset(off_t element_offset)
        {
            off_t offset = element_offset;
            if (offset + sizeof(TKey) + sizeof(TValue) > bucket_data_bytes) 
                throw std::out_of_range("Bad table key offset--reading value passes end of buffer.");

            return offset;
        }


        off_t inline value_offset(off_t element_offset)
        {
            off_t offset = element_offset + sizeof(TKey);
            if (offset + sizeof(TValue) > bucket_data_bytes)
                throw std::out_of_range("Bad table value offset--reading value passes end of buffer.");

            return offset;
        }


        // quick zeroed memory check, thanks to Accipitridae on Stack Overflow
        // I'm not sure if the length of the buffer has a huge effect on perf 
        // here. It might be worth while to first check the key, and then only
        // check the value if the key is zeroed. I suppose we could check the
        // key and the key alone, but that would preclude storing 0 as a key. 
        // This approach only precludes storing {0, 0} as a KVP.
        //
        // All this could be avoided by increasing the element size to
        // accommodate a status flag of some kind. I suppose that is probably
        // worth it.
        bool inline is_empty(off_t element_offset, byte* bucket)
        {
            if (element_offset + element_sz > bucket_data_bytes)
                throw std::out_of_range("Bad table element offset--reading value passes end of buffer.");

            // If the first element is 0, and each element in the buffer
            // matches with the following element for the whole length, than
            // the buffer must be zeroed out. I may have failed at this sort of
            // alignment matching in kindergarten, but I have a computer now!
            return (bucket[element_offset] == 0) && (!memcmp(bucket + element_offset, 
                    bucket + element_offset + 1, element_sz - 1));
        }


        void inline prepare_element(byte *element, TKey key, TValue val)
        {
            memcpy(element, &key, sizeof(TKey));
            memcpy(element + sizeof(key), &val, sizeof(val));
        }


    public:
        HashTable(size_t bucket_cnt)
        {
            //this->storage = std::make_unique<MemIOHandler>(1024);
            this->storage = new MemIOHandler(128);
            byte x = 0;
            storage->write(&x, 1, bucket_cnt * bucket_bytes - 1);
            this->bucket_cnt = bucket_cnt;
        }


        TValue insert(TKey key, TValue val)
        {
            off_t offset = get_bucket(key);
            off_t insert_offset = -1;
            off_t insert_bucket = offset;

            bool more_chain = true;
            byte bucket[bucket_bytes] = {0};

            while (more_chain) {
                this->storage->read(bucket, bucket_bytes, offset);
                for (size_t i=0; i<bucket_data_bytes; i+=element_sz) {
                    int result = memcmp(&key, bucket + key_offset(i), sizeof(TKey));
                    if (result == 0) {
                        // the key is already present in table
                        TValue retval;
                        memcpy(&retval, bucket + value_offset(i), sizeof(TValue));
                        return retval;
                    } else if (result > 0 && insert_offset == -1 && is_empty(i, bucket)) {
                        // As we're iterating over the chain, we may as well 
                        // locate the first empty spot where we *could* stick
                        // the element, if we end up needing to insert it. By
                        // sticking it in the first available spot, rather than
                        // at the end, we can easily fill in holes left by 
                        // deletions.
                        insert_offset = i;
                        insert_bucket = offset;
                    }
                }

                off_t *next_offset = (off_t *) (bucket + bucket_data_bytes);
                if (*next_offset == 0) {
                    more_chain = false;
                } else {
                    offset = *next_offset;
                }
            }

            // The key isn't in the table, so we need to write it.
            byte element[element_sz];
            prepare_element(element, key, val);

            if (insert_offset != -1) {
                // the key doesn't exist, and a valid insertion spot was found
                off_t write_offset = insert_bucket + insert_offset;
                this->storage->write(element, element_sz, write_offset);
            } else {
                // the key doesn't exist, and we need to add a new link to the
                // chain to write it.
                off_t write_offset = this->storage->get_flen();
                this->storage->write(element, element_sz, write_offset);
                byte x = 0;
                
                // Ensuring that the length of the memory region is an even
                // multiple of bucket_bytes. Talk about fighting with the
                // limitations of an API...
                this->storage->write(&x, 1, write_offset + this->bucket_bytes);

                // update the offset in the previous chain link
                this->storage->write((byte *) &write_offset, sizeof(off_t), 
                        offset + bucket_data_bytes);
            }

            return val;
        }

        
        TValue get(TKey key)
        {
            off_t offset = get_bucket(key); 

            bool more_chain = true;
            byte bucket[bucket_bytes] = {0};

            while (more_chain) {
                this->storage->read(bucket, bucket_bytes, offset);
                for (size_t i=0; i<bucket_data_bytes; i+=element_sz) {
                    int result = memcmp(&key, bucket + key_offset(i), sizeof(TKey));
                    if (result == 0) {
                        TValue retval;
                        memcpy(&retval, bucket + value_offset(i), sizeof(TValue));
                        return retval;
                    } 
                }

                off_t *next_offset = (off_t *) (bucket + bucket_data_bytes);
                if (*next_offset == 0) {
                    more_chain = false;
                } else {
                    offset = *next_offset;
                }
            }

            // element not in the table
            throw KeyNotFoundException();
        }


        void remove(TKey key)
        {
            off_t offset = get_bucket(key); 

            bool more_chain = true;
            byte bucket[bucket_bytes] = {0};

            while (more_chain) {
                this->storage->read(bucket, bucket_bytes, offset);
                for (size_t i=0; i<bucket_data_bytes; i+=element_sz) {
                    int result = memcmp(&key, bucket + key_offset(i), sizeof(TKey));
                    if (result == 0) {
                        byte zeroes[element_sz];
                        memset(zeroes, 0, element_sz);
                        this->storage->write(zeroes, element_sz, offset + key_offset(i));
                        return;
                    } 
                }

                off_t *next_offset = (off_t *) (bucket + bucket_data_bytes);
                if (*next_offset == 0) {
                    more_chain = false;
                } else {
                    offset = *next_offset;
                }
            }

            // element not in the table
            throw KeyNotFoundException();
        }



        size_t hash(TKey key) 
        {
            std::hash<TKey> hash_key;
            size_t val = hash_key(key);
            return val % bucket_cnt;
        }


        size_t get_bucket_count()
        {
            return this->bucket_cnt;
        }


        IOHandler *get_io_handler()
        {
            return this->storage;
        }

        ~HashTable() 
        {
            delete this->storage;
        }

};
