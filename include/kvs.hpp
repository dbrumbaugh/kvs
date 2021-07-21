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
typedef int fd_t;

#define CACHELINE 64
#define REDUCED_CACHELINE CACHELINE - sizeof(off_t)



#endif
