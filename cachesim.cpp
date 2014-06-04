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

void update_overhead(int cache_ind, int block_ind, uint64_t set_ind, bool replaced) { 
    
    printf("Index: %d\n", block_ind);

    if (info.R == true) { /* LRU */
        //printf("In LRU\n");
        int old_lru = cache[cache_ind].sets[set_ind].blocks[block_ind].num_lru;

        for (int i = 0; i < info.num_blocks_set; i++) {
            if (cache[cache_ind].sets[set_ind].blocks[block_ind].num_lru > old_lru) 
                cache[cache_ind].sets[set_ind].blocks[block_ind].num_lru -= 1;
            if (i == block_ind)
                cache[cache_ind].sets[set_ind].blocks[block_ind].num_lru = 
                    info.num_blocks_set - 1;
        }
        //printf("Out LRU\n");
    }
    else { /* NMRU-FIFO */
        //printf("In FIFO\n");
        for (int i = 0; i < info.num_blocks_set; i = i + 1) {
            //printf("i: %d\n", i);
            if (i == block_ind) {
                cache[cache_ind].sets[set_ind].blocks[block_ind].mru = true;
            }
            else {
                cache[cache_ind].sets[set_ind].blocks[i].mru = false;
            }
        }
        if (replaced) {
            cache[cache_ind].sets[set_ind].blocks[block_ind].num_fifo = 
                info.num_blocks_set -1;
        }
        //printf("Out FIFO\n");
    }
}

int get_replacement_block(int cache_ind, int set_ind) {

    int ret_ind;

    if (info.R == true) { /* LRU */
        for (int i = 0; i < info.num_blocks_set; i++) {
            if (cache[cache_ind].sets[set_ind].blocks[i].num_lru == 0) {
                return i;
            }
        }
    }
    else { /* NMRU-FIFIO */
        int first, second, decrement_start_ind;

        /* Debug: Print set */
        printf("Set[%d]\n", set_ind);
        for (int i = 0; i < info.num_blocks_set; i++) {
            printf("\tBlock[%d]", i);
            printf("\t\tTag: %" PRIu64 "\n", cache[cache_ind].sets[set_ind].blocks[i].tag);
            if (cache[cache_ind].sets[set_ind].blocks[i].mru)
                printf("\t\tMRU: true\n");
            else
                printf("\t\tMRU: false\n");
            printf("\t\tNum_FIFO: %d\n", cache[cache_ind].sets[set_ind].blocks[i].num_fifo);
        }
        /**/

        /* Find the first and second blocks in */
        for (int i = 0; i < info.num_blocks_set; i++) {
            if (cache[cache_ind].sets[set_ind].blocks[i].num_fifo == 0)
                first = i;
            if (cache[cache_ind].sets[set_ind].blocks[i].num_fifo == 1)
                second = i;
        }

        printf("First: %d", first);
        printf("\tSecond: %d\n", second);

        /* Decrement all blocks with a num_fifo that is greater than 2 or 1
            depending on whether the first in block is the mru block */
        decrement_start_ind = cache[cache_ind].sets[set_ind].blocks[first].mru == true ? 2 : 1;
        printf("Decrement: %d\n", decrement_start_ind);
        for (int i = 0; i < info.num_blocks_set; i++) {
            /* Decrement all blocks depending on whether MRU is set or not */
            if (cache[cache_ind].sets[set_ind].blocks[i].num_fifo >= decrement_start_ind)
                cache[cache_ind].sets[set_ind].blocks[i].num_fifo -= 1;
        }

        /* Debug */
        if (decrement_start_ind == 1) {
            printf("Replace Block: %d\n", first);
            return first;
        }
        else {
            printf("Replace Block: %d\n", second);
            return second;
        }

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

        printf("info.ST: %s\n", info.ST ? "true" : "false");
        printf("info.R: %s\n", info.R ? "true" : "false");
        printf("Total Bytes %" PRIu64 "\n", info.total_bytes);
        printf("Bytes Per Block %" PRIu64 "\n", info.bytes_per_block);
        printf("Num Sets %" PRIu64 "\n", info.num_blocks_set);
        printf("Num Lines %" PRIu64 "\n", info.num_sets);
        printf("t: %" PRIu64 "\n", info.tag_bits);
        printf("o: %" PRIu64 "\n", info.offset_bits);
        printf("i: %" PRIu64 "\n", info.index_bits);

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
            sets[i].blocks[j].tag = 0;
            sets[i].blocks[j].dirty = false;
            sets[i].blocks[j].valid = false;
            sets[i].blocks[j].valid_first = false;
            sets[i].blocks[j].valid_second = false;
            sets[i].blocks[j].num_lru = 0;
            sets[i].blocks[j].mru = false;
            sets[i].blocks[j].num_fifo = j;
        }
    }

    one_cache.sets = sets;

    cache.push_back(one_cache);

    for (int i = 0; i < info.num_processors; i++) {
        printf("Cache[%d]\n", i);
        for (int j = 0; j < info.num_sets; j++) {
            printf("\tSet[%d]\n", j);
            for (int k = 0; k < info.num_blocks_set; k++) {
                printf("\t\tBlocks[%d]\n", k);
                printf("\t\t\tTag: %" PRIu64 "\n", cache[i].sets[j].blocks[k].tag);
                printf("\t\t\tDirty: %d\n", cache[i].sets[j].blocks[k].dirty);
                printf("\t\t\tValid: %d\n", cache[i].sets[j].blocks[k].valid);
                printf("\t\t\tValid1: %d\n", cache[i].sets[j].blocks[k].valid_first);
                printf("\t\t\tValid2: %d\n", cache[i].sets[j].blocks[k].valid_second);
                printf("\t\t\tNum_LRU: %d\n", cache[i].sets[j].blocks[k].num_lru);
                printf("\t\t\tMRU: %d\n", cache[i].sets[j].blocks[k].mru);
                printf("\t\t\tNum_FIFO: %d\n", cache[i].sets[j].blocks[k].num_fifo);
            }
        }
    }

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

    int index = -1;

    /* For the first access, we have the data structure but the ctid not filled in */
    if (ctid_init == 1) {
        cache[0].ctid = ctid;
        ctid_init = 0;
    }

    /* Go through the vector and see if we already have the ctid */
    for (int i = 0; i < cache.size(); i++) {
        /* if we have it then just set the index */
        if (cache[i].ctid == ctid) {
            index = i;
            break;
        }
    }

    /* if we don't have it already then we need to create it */
    if (index == -1) {
        setup_cache(info.C, info.B, info.S, info.ST, info.R);
        cache[index = cache.size()-1].ctid = ctid;
    }

    p_stats->accesses++;

    if (rw == WRITE) {
        printf("\nCache Write\n");
        p_stats->writes++;
        cache_write(index, numOfBytes, address, p_stats);
    }
    else {
        printf("\nCache Read\n");
        p_stats->reads++;
        cache_read(index, numOfBytes, address, p_stats);
    }
}

void cache_write(int cache_index, char numOfBytes, address_t address, cache_stats_t* p_stats) {

}

void cache_read(int cache_index, char numOfBytes, address_t address, cache_stats_t* p_stats) {

/* Check for a hit */
    /* Get the index and tag */
    uint64_t tag = parse_address(address, 't');
    uint64_t index = parse_address(address, 'i');
    uint64_t offset = parse_address(address, 'o');

    int num_to_access = (int) ceil(((double) offset + (double) numOfBytes) /
                            (double) info.bytes_per_block);
    
    printf("CTID: %u\n", cache[cache_index].ctid);
    //printf("Address: %" PRIu64 "\n", address);
    printf("Tag: %" PRIu64 "\n", tag);
    printf("Index: %" PRIu64 "\n", index);
    printf("Offset: %" PRIu64 "\n", offset);
    printf("NumBytes: %d\n", numOfBytes);
    printf("NumAccess: %d\n", num_to_access);
    /**/                                

    int new_index = 0;
    int i, j;

    for (i = 0; i < num_to_access; i++) {

        bool missed = true;
        /* Make sure not to go out of bounds, need to do modulus */
        new_index = (i+index) % info.num_sets;

        //printf("Access: %d\n", new_index);

        /* Loop through the cache */
        for (j = 0; j < info.num_blocks_set; j++) {
            //printf("Block: %d\n", j);
            if (cache[cache_index].sets[new_index].blocks[j].tag == tag) {
                if (cache[cache_index].sets[new_index].blocks[j].valid) {
                    printf("Hit in Block: %d\n", j);
                    update_overhead(cache_index, j, (uint64_t) new_index, false);
                    missed = false;
                    break;
                }
            }
        }

        /* Miss for this particular access */ 
        if (missed) {
            printf("Miss\n");
            p_stats->read_misses_combined += 1;
            p_stats->misses += 1;
            if (cache_index == 0)
                p_stats->read_misses +=1;

            /* Get a block to replace */
            int ind = get_replacement_block(cache_index, new_index);

            /* Update overhead */
            cache[cache_index].sets[new_index].blocks[ind].tag = tag;
            if (info.ST) {
                /* Figure out which half it is */
                int half = ceil((info.num_blocks_set - 1) / 2);
                /* Update the first half */
                if (offset < half) {
                    cache[cache_index].sets[new_index].blocks[ind].valid_first = true;
                    cache[cache_index].sets[new_index].blocks[ind].valid_second = false;

                    /* If the number of bytes we want spans to the other half then we
                    need to update that one too. It will be another miss */
                    if (offset + numOfBytes > half) {
                        cache[cache_index].sets[new_index].blocks[ind].valid_second = true;
                        //p_stats->read_misses += 1;
                    }
                }
                else {
                    cache[cache_index].sets[new_index].blocks[ind].valid_second = true;
                    cache[cache_index].sets[new_index].blocks[ind].valid_first = false;
                }
            }
            else {
                cache[cache_index].sets[new_index].blocks[ind].valid = true;
            }
            cache[cache_index].sets[new_index].blocks[ind].valid = true;
            cache[cache_index].sets[new_index].blocks[ind].dirty = false;
            update_overhead(cache_index, ind, new_index, true);

            /* Debug: Print set */
            printf("Set[%d]\n", new_index);
            for (int i = 0; i < info.num_blocks_set; i++) {
                printf("\tBlock[%d]", i);
                printf("\t\tTag: %" PRIu64 "\n", cache[cache_index].sets[new_index].blocks[i].tag);
                if (cache[cache_index].sets[new_index].blocks[i].mru)
                    printf("\t\tMRU: true\n");
                else
                    printf("\t\tMRU: false\n");
                printf("\t\tNum_FIFO: %d\n", cache[cache_index].sets[new_index].blocks[i].num_fifo);
            }
            /**/

        }
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
