/*
 * kvs.hpp
 * KVS generic definitions
 *
 * Douglas Rumbaugh, 7-16-21
 *
 */

#ifndef kvs
#define kvs

typedef char byte;
static_assert(sizeof(byte) == 1, "Byte-type must have a size of 1");
typedef int fd_t;

#define CACHELINE 64
#define REDUCED_CACHELINE CACHELINE - sizeof(off_t)
#define PAGESIZE 4096



#endif
