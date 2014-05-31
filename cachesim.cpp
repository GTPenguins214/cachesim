#include <stdio.h>
#include "cachesim.hpp"

cache_set_t *sets;
cache_info_t info;

/**
 * Subroutine for initializing the cache. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @c The total number of bytes for data storage is 2^C
 * @b The size of a single cache line in bytes is 2^B
 * @s The number of blocks in each set is 2^S
 * @st The storage policy, BLOCKING or SUBBLOCKING (refer to project description for details)
 * @r The replacement policy, LRU or NMRU_FIFO (refer to project description for details)
 */
void setup_cache(uint64_t c, uint64_t b, uint64_t s, char st, char r) {

    printf("Got to beginning of cachesim:setup_cache\n");

    /* Fill in the cache_info data structure */
    info.total_bytes = 1 << c;
    info.bytes_per_block = 1 << b;
    info.num_sets = 1 << s;
    info.num_lines = info.total_bytes / (info.num_sets*info.bytes_per_block);
    info.tag_bits = ADDRESS_SIZE - info.offset_bits - info.index_bits;
    info.index_bits = c-s-b;
    info.offset_bits = b;
    info.S = s;
    info.B = b;
    info.C = c;
    switch(st) {
        case BLOCKING:
            info.ST = 0;
            break;
        case SUBBLOCKING:
            info.ST = 1;
            break;
    }

    switch(r) {
        case LRU:
            info.R = 0;
            break;
        case NMRU_FIFO:
            info.R = 1;
            break;
    }

    /* Create the cache_set_t data structure */
    sets = new cache_set_t[info.num_sets];

    for (int i = 0; i < info.num_sets; i++) {
        sets[i].blocks = new cache_block_t[info.num_lines];
    }

    printf("Got to end of cachesim:setup_cache\n");

}
/**
 * Subroutine that simulates the cache one trace event at a time.
 * XXX: You're responsible for completing this routine
 *
 * @rw The type of event. Either READ or WRITE
 * @address  The target memory address
 * @p_stats Pointer to the statistics structure
 */
void cache_access(unsigned int ctid, char rw, char numOfBytes, uint64_t address, cache_stats_t* p_stats) {

    p_stats->accesses++;
    
    if (rw == WRITE)
    {
        p_stats->writes++;

    }
    else
    {
        p_stats->reads++;
    }
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_cache(cache_stats_t *p_stats) {

    for (int i = 0; i < info.num_sets; i++) {
        delete[] sets[i].blocks;
    }
    delete[] sets;
}
