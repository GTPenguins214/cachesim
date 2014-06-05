#include <vector>
#include <stdio.h>
#include <math.h>
#include "cachesim.hpp"

using std::vector;

vector <cache_t> cache;
cache_info_t info;
int ctid_init = 1;

uint64_t parse_address(address_t addr, char part) {

    address_t ret_addr;

    switch (part) {
        case 't':
            /* Just shift over the amount of bits */
            ret_addr = addr >> (info.offset_bits + info.index_bits);
            break;
        case 'i':
            /* Shift out the offset its but then use masking to 
            take out the tag bits.  */
            ret_addr = addr >> (info.offset_bits);
            ret_addr = ret_addr & (info.num_sets - 1);
            break;
        case 'o':
            /* Bitmask out the index and tag */
            ret_addr = addr &  ((uint64_t) ((uint64_t)1 <<(info.B)) - 1);
            break;
    }

    return ret_addr;
}

void update_policy(int cache_ind, uint64_t set_ind, int block_ind) {
    if (info.R = true) { /* LRU */
        printf("LRU\n");
        int old_lru = cache[cache_ind].sets[set_ind].blocks[block_ind].num_lru;

        for (int i = 0; i < info.num_blocks_set; i++) {
            if (cache[cache_ind].sets[set_ind].blocks[i].num_lru > old_lru) {
                cache[cache_ind].sets[set_ind].blocks[i].num_lru -= 1;
                printf("Block %d is %d\n", i, cache[cache_ind].sets[set_ind].blocks[i].num_lru);
            }
            if (i == block_ind) {
                cache[cache_ind].sets[set_ind].blocks[block_ind].num_lru = 
                    info.num_blocks_set - 1;
                printf("Block %d is %d\n", i, cache[cache_ind].sets[set_ind].blocks[i].num_lru);
            }
        }
    }
    else { /* NMRU-FIFO */
        printf("NMRU-FIFO\n");
    }
}

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

    if (ctid_init == 1) {
        /* Fill in the cache_info data structure */
        info.total_bytes = (uint64_t) ((uint64_t)1 << c);
        info.bytes_per_block = (uint64_t) ((uint64_t)1 << b);
        info.num_blocks_set = (uint64_t) ((uint64_t)1 << s);
        info.num_sets = info.total_bytes / (info.num_blocks_set*info.bytes_per_block);
        info.index_bits = c-s-b;
        info.offset_bits = b;
        info.tag_bits = ADDRESS_SIZE - info.offset_bits - info.index_bits;
        info.S = s;
        info.B = b;
        info.C = c;
        info.ST = (st == 'S') ? true : false; /* true = sub, false = block */
        info.R = (r == 'L') ? true : false; /* true = lru, false = fifo */

        printf("info.ST: %s\n", info.ST ? "Subblocking" : "Blocking");
        printf("info.R: %s\n", info.R ? "LRU" : "NMRU-FIFO");
        printf("Total Bytes %" PRIu64 "\n", info.total_bytes);
        printf("Bytes Per Block %" PRIu64 "\n", info.bytes_per_block);
        printf("Num Sets %" PRIu64 "\t", info.num_blocks_set);
        printf("Num Lines %" PRIu64 "\n", info.num_sets);
        printf("Tag Bits: %" PRIu64 "\t", info.tag_bits);
        printf("Index Bits: %" PRIu64 "\t", info.index_bits);
        printf("Offset Bits: %" PRIu64 "\n", info.offset_bits);

    }

    info.num_processors += 1;

    cache_t one_cache;
    one_cache.ctid = 0;

    /* Create the cache set_t data structure */
    vector <cache_set_t> sets(info.num_sets);
    for (int i = 0; i < info.num_sets; i++) {
        vector <cache_block_t> blocks(info.num_blocks_set);
        sets[i].blocks = blocks;
        for (int j = 0; j < info.num_blocks_set; j++) {
            sets[i].blocks[j].tag = -1;
            sets[i].blocks[j].dirty = false;
            sets[i].blocks[j].valid = false;
            sets[i].blocks[j].valid_first = false;
            sets[i].blocks[j].valid_second = false;
            sets[i].blocks[j].num_lru = j;
            sets[i].blocks[j].mru = false;
            sets[i].blocks[j].num_fifo = j;
        }
    }

    one_cache.sets = sets;

    cache.push_back(one_cache);

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

    int cache_index = -1;

    /* For the first access, we have the data structure but the ctid not filled in */
    if (ctid_init == 1) {
        cache[0].ctid = ctid;
        ctid_init = 0;
    }

    /* Go through the vector and see if we already have the ctid */
    for (int i = 0; i < cache.size(); i++) {
        /* if we have it then just set the index */
        if (cache[i].ctid == ctid) {
            cache_index = i;
            break;
        }
    }

    /* if we don't have it already then we need to create it */
    if (cache_index == -1) {
        setup_cache(info.C, info.B, info.S, info.ST, info.R);
        cache[cache_index = cache.size()-1].ctid = ctid;
    }

    /* Get the tag, index and offset */
    uint64_t tag = parse_address(address, 't');
    uint64_t index = parse_address(address, 'i');
    uint64_t offset = parse_address(address, 'o');

    /* Figure out if the number of bytes spans multiple cache lines */
    int num_set_access = (int) ceil(((double) offset + (double) numOfBytes) /
                            (double) info.bytes_per_block);

    p_stats->accesses++;
    
    if (rw == WRITE)
    {
        /* Debug */
        
        printf("\nWrite\n");
        printf("CTID: %d\n", ctid);
        printf("Tag: %" PRIu64 "\t", tag);
        printf("Index: %" PRIu64 "\t", index);
        printf("Offset: %" PRIu64 "\n", offset);
        printf("NumOfBytes: %d\t", numOfBytes);
        printf("Num_Set_Access: %d\n", num_set_access);
        /**/

        p_stats->writes++;

        /* Call the function as many times as num_set_access */
        for (int i = 0; i < num_set_access; i++) {
            int set_index = (i + index) % info.num_sets;
            printf("Set Index: %d\n", set_index);
            cache_write(cache_index, tag, set_index, offset, p_stats);
        }
        
    }
    else
    {
        /* Debug */
        
        printf("\nRead\n");
        printf("CTID: %d\n", ctid);
        printf("Tag: %" PRIu64 "\t", tag);
        printf("Index: %" PRIu64 "\t", index);
        printf("Offset: %" PRIu64 "\n", offset);
        printf("NumOfBytes: %d\t", numOfBytes);
        printf("Num_Set_Access: %d\n", num_set_access);
        /**/

        p_stats->reads++;

        /* Call the function as many times as num_set_access */
        for (int i = 0; i < num_set_access; i++) {
            int set_index = (i + index) % info.num_sets;
            printf("Set Index; %d\n", set_index);
            cache_read(cache_index, tag, set_index, offset, p_stats);
        }
    }
}

void cache_write(int cache_index, uint64_t tag, uint64_t index, 
                    uint64_t offset, cache_stats_t* p_stats) {

}

void cache_read(int cache_index, uint64_t tag, uint64_t index, 
                    uint64_t offset, cache_stats_t* p_stats) {

}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_cache(cache_stats_t *p_stats) {

    for (int i = 0; i < cache.size(); i++) {
        printf("Index: %d\t", i);
        printf("CTID: %u\n", cache[i].ctid);
    }

    for (int i = 0; i < info.num_processors; i++) {
        for (int j = 0; j < info.num_sets; j++) {
            cache[i].sets[j].blocks.clear();
        }
        cache[i].sets.clear();
    }

    cache.clear();

}
