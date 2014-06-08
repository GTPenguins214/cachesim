#include <vector>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "cachesim.hpp"

using std::vector;

vector <cache_t> cache;
cache_info_t info;
int ctid_init = 1;

bool log_off = true;

FILE *myfile0, *myfile1, *myfile2, *myfile3, *myfile4, *myfile5, *myfile6, *myfile7;

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
            ret_addr = addr &  ((uint64_t) ((uint64_t)1 << (info.B)) - 1);
            break;
    }

    return ret_addr;
}

void update_policy(int cache_ind, uint64_t set_ind, int block_ind, bool replaced) {
    if (info.R = true) { /* LRU */
        int old_lru = cache[cache_ind].sets[set_ind].blocks[block_ind].num_lru;

        for (int i = 0; i < info.num_blocks_set; i++) {
            if (cache[cache_ind].sets[set_ind].blocks[i].num_lru > old_lru) {
                cache[cache_ind].sets[set_ind].blocks[i].num_lru -= 1;
            }
            if (i == block_ind) {
                cache[cache_ind].sets[set_ind].blocks[block_ind].num_lru = 
                    info.num_blocks_set - 1;
            }
        }
    }
    else { /* NMRU-FIFO */
        ///printf("NMRU-FIFO\n");
        for (int i = 0; i < info.num_blocks_set; i = i + 1) {
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
    }
}

int get_replacement_block(int cache_ind, uint64_t set_ind) {
    if (info.R) {
        /* Get the first block with an LRU of 0 */
        for (int i = 0; i < info.num_blocks_set; i++) {
            if (cache[cache_ind].sets[set_ind].blocks[i].num_lru == 0) {
                return i;
            }
        }

        /* Throw an assert */
        printf("\nREPLACEMENT ALGORITHM FAILED!!!!\n");
        assert(false);
    }
    else { /*NMRU-FIFO */
        int first, second, decrement_start_ind;

        /* Find the index of the blocks with num_fifo=0 and 1 */
        for (int i = 0; i < info.num_blocks_set; i++) {
            if (cache[cache_ind].sets[set_ind].blocks[i].num_fifo == 0)
                first = i;
            if (cache[cache_ind].sets[set_ind].blocks[i].num_fifo == 1)
                second = i;
        }

        decrement_start_ind = cache[cache_ind].sets[set_ind].blocks[first].mru == true ? 2 : 1;
        for (int i = 0; i < info.num_blocks_set; i++) {
            /* Decrement all blocks depending on whether MRU is set or not */
            if (cache[cache_ind].sets[set_ind].blocks[i].num_fifo >= decrement_start_ind) {
                cache[cache_ind].sets[set_ind].blocks[i].num_fifo -= 1;
                ///printf("Changing block %d to num_fifo = %d\n", i,
                    ///cache[cache_ind].sets[set_ind].blocks[i].num_fifo);
            }
        }

        if (decrement_start_ind == 1) {
            ///printf("Replace Block: %d\n", first);
            return first;
        }
        else {
            ///printf("Replace Block: %d\n", second);
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

        printf("info.ST: %s\n", info.ST ? "Subblocking" : "Blocking");
        printf("info.R: %s\n", info.R ? "LRU" : "NMRU-FIFO");
        printf("Total Bytes %" PRIu64 "\n", info.total_bytes);
        printf("Bytes Per Block %" PRIu64 "\n", info.bytes_per_block);
        printf("Num Blocks Set %" PRIu64 "\t", info.num_blocks_set);
        printf("Num Lines %" PRIu64 "\n", info.num_sets);
        printf("Tag Bits: %" PRIu64 "\t", info.tag_bits);
        printf("Index Bits: %" PRIu64 "\t", info.index_bits);
        printf("Offset Bits: %" PRIu64 "\n", info.offset_bits);

        if (!log_off) {
            myfile0 = fopen("output0.txt", "w");
            myfile1 = fopen("output1.txt", "w");
            myfile2 = fopen("output2.txt", "w");
            myfile3 = fopen("output3.txt", "w");
            myfile4 = fopen("output4.txt", "w");
            myfile5 = fopen("output5.txt", "w");
            myfile6 = fopen("output6.txt", "w");
            myfile7 = fopen("output7.txt", "w");
        }
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
            sets[i].blocks[j].num_lru = 0;
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

    int cache_ind = -1;

    /* For the first access, we have the data structure but the ctid not filled in */
    if (ctid_init == 1) {
        info.num_processors = 1;
        cache[0].ctid = ctid;
        ctid_init = 0;
    }

    /* Go through the vector and see if we already have the ctid */
    for (int i = 0; i < cache.size(); i++) {
        /* if we have it then just set the index */
        if (cache[i].ctid == ctid) {
            cache_ind = i;
            break;
        }
    }

    /* if we don't have it already then we need to create it */
    if (cache_ind == -1) {
        setup_cache(info.C, info.B, info.S, info.ST, info.R);
        cache_ind = cache.size() -1;
        cache[cache_ind].ctid = ctid;
    }

    /* Just some arbitrary number that probably won't get hit. */
    uint64_t old_index = 1000000;
    /* Use this to tell when to count misses and hits */
    bool count_it = true;

    p_stats->accesses++;

    if (rw == WRITE)
        p_stats->writes++;
    else
        p_stats->reads++;

    /* Loop over the number of bytes we want */
    for (int i = 0; i < numOfBytes; i++) {
        /* Get the tag, index and offset */
        uint64_t tag = parse_address(address, 't');
        uint64_t index = parse_address(address, 'i');
        uint64_t offset = parse_address(address, 'o');

        /* The main cache access function */
        bool is_miss = cache_lookup(ctid, cache_ind, tag, index, offset, p_stats, count_it, rw);

        /* If we get a miss then stop keeping track because we only count multiple
        misses as one miss */
        if (is_miss) 
            count_it = false;

        /* Increment the address and set count_it to false */
        address += 1;
    }

}

bool cache_lookup(int ctid, int cache_ind, uint64_t tag, uint64_t index, 
                 uint64_t offset, cache_stats_t* p_stats, bool count_it, char rw) {

    bool first = false, second = false;
    bool missed = true; /* Assume we miss */

    /* Check if we need half blocks */
    if (info.ST) {

        /* If the offset is in the lower half then we need it */
        if (offset < ceil(info.bytes_per_block/2)) {
            first = true;
        }

        /* If it is in the upper half */
        if (offset >= ceil(info.bytes_per_block/2)) {
            second = true;
        }

        for (int i = 0; i < info.num_blocks_set; i++) {
            /* Get the tag and cmopare */
            if (cache[cache_ind].sets[index].blocks[i].tag == tag) {
                if (first) { /* If we need to check the first */
                    /* Check its validity */
                    if (cache[cache_ind].sets[index].blocks[i].valid_first)
                        missed = false;
                    else {
                        /* It's invalid so bring it in */
                        cache[cache_ind].sets[index].blocks[i].valid_first = true;
                        if (count_it) {
                            if (rw == WRITE) {
                                if (ctid == 0)
                                    p_stats->write_misses += 1;
                                p_stats->write_misses_combined += 1;
                            }
                            else {
                                if (ctid == 0)
                                    p_stats->read_misses += 1;
                                p_stats->read_misses_combined += 1;
                            }
                            
                        }
                        missed = true;
                    }
                }
                else { /* Only need to check the second block */
                    /* If it is invalid then bring it in and count a miss */
                    if (cache[cache_ind].sets[index].blocks[i].valid_second)
                        missed = false;
                    else {
                        cache[cache_ind].sets[index].blocks[i].valid_second = true;
                        if (count_it) {
                            if (rw == WRITE) {
                                if (ctid == 0) 
                                    p_stats->write_misses += 1;
                                p_stats->write_misses_combined += 1;
                            }
                            else {
                                if (ctid == 0) 
                                    p_stats->read_misses += 1;
                                p_stats->read_misses_combined += 1;
                            }
                            
                        }
                        missed = true;
                    }
                }
                update_policy(cache_ind, index, i, false);
                return missed;
            }
        }

        if (missed) {
            /* Update the stats */
            if (count_it) {
                if (rw == WRITE) {
                    if (ctid == 0)
                        p_stats->write_misses += 1;
                    p_stats->write_misses_combined += 1;
                }
                else {
                    if (ctid == 0)
                        p_stats->read_misses += 1;
                    p_stats->read_misses_combined += 1;
                }
            }

            /* Get a replacement block */
            int i = get_replacement_block(cache_ind, index);

            /* Replace the block */
            cache[cache_ind].sets[index].blocks[i].tag = tag;
            if (rw == WRITE)
                cache[cache_ind].sets[index].blocks[i].dirty = true;
            else
                cache[cache_ind].sets[index].blocks[i].dirty = false;
            
            if (first) {
                cache[cache_ind].sets[index].blocks[i].valid_first = true;
                cache[cache_ind].sets[index].blocks[i].valid_second = false;
            }
            else {
                cache[cache_ind].sets[index].blocks[i].valid_second = true;
                cache[cache_ind].sets[index].blocks[i].valid_first = false;
            }

            update_policy(cache_ind, index, i, true);
            return true;
        }
    }
    else { /* Blocking */

        /* Loop through the blocks in the set and see if we have a hit */
        for (int block_ind = 0; block_ind < info.num_blocks_set; block_ind++ ) {
            /* Get the tag and compare */
            if (cache[cache_ind].sets[index].blocks[block_ind].tag == tag) {

                /* Update the replacement policy */
                update_policy(cache_ind, index, block_ind, false);

                /* Set the block to dirty */
                if (rw == WRITE) {
                    cache[cache_ind].sets[index].blocks[block_ind].dirty = true;
                    //if (count_it)
                        log_output(0, ctid, cache_ind, tag, index, block_ind, 'H', 'W');
                }
                else {
                    //if (count_it)
                        log_output(0, ctid, cache_ind, tag, index, block_ind, 'H', 'R');
                }

                missed = false;
                return false;

            }
        }

        if (missed) {

            if (count_it) {
                if (rw == WRITE) {
                    if (ctid == 0) {
                        p_stats->write_misses += 1;
                    }
                    p_stats->write_misses_combined += 1;

                    log_output(0, ctid, cache_ind, tag, index, 0, 'M', 'W');
                }
                else {
                    if (ctid == 0) {
                        p_stats->read_misses += 1;
                    }
                    p_stats->read_misses_combined += 1;

                    log_output(0, ctid, cache_ind, tag, index, 0, 'M', 'R');
                }
            }

            /* Get a replacement block */
            int block_ind = get_replacement_block(cache_ind, index);
            log_output(1, ctid, cache_ind, tag, index, block_ind, 'M', 'R');

            /* Replace the block */
            cache[cache_ind].sets[index].blocks[block_ind].tag = tag;
            cache[cache_ind].sets[index].blocks[block_ind].valid = true;
            if (rw == WRITE)
                cache[cache_ind].sets[index].blocks[block_ind].dirty = true;
            else
                cache[cache_ind].sets[index].blocks[block_ind].dirty = false;

            /* Update the replacement policy */
            update_policy(cache_ind, index, block_ind, true);

            return true;
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

    for (int i = 0; i < info.num_processors; i++) {
        for (int j = 0; j < info.num_sets; j++) {
            cache[i].sets[j].blocks.clear();
        }
        cache[i].sets.clear();
    }

    if (!log_off) {
        fclose(myfile0);
        fclose(myfile1);
        fclose(myfile2);
        fclose(myfile3);
        fclose(myfile4);
        fclose(myfile5);
        fclose(myfile6);
        fclose(myfile7);
    }

    cache.clear();

}

void log_output(int output_type, int ctid, int cache_index, uint64_t tag, 
                uint64_t set_index, uint64_t block_index,
                char hit_miss, char read_write) {

    if (log_off)
        return;

    FILE * myfile;

    switch (cache_index) {
        case 0:
            myfile = myfile0;
            break;
        case 1:
            myfile = myfile1;
            break;
        case 2:
            myfile = myfile2;
            break;
        case 3:
            myfile = myfile3;
            break;
        case 4:
            myfile = myfile4;
            break;
        case 5:
            myfile = myfile5;
            break;
        case 6:
            myfile = myfile6;
            break;
        case 7:
            myfile = myfile7;
            break;
    }

    switch (output_type) {
        /* Print hit or miss */
        case 0:
            if (read_write == 'R')
                fprintf(myfile, "R\t");
            else
                fprintf(myfile, "W\t");

            fprintf(myfile, "Tag: %" PRIu64 "\t", tag);
            fprintf(myfile, "Set: %" PRIu64 "\t", set_index);
            
            if (hit_miss == 'H') {
                fprintf(myfile, "Block: %" PRIu64 "\t", block_index);
                fprintf(myfile, "Hit\t");
            }
            else
                fprintf(myfile, "Miss\t");

            break;   

        /* Print block to replace */
        case 1:
            fprintf(myfile, "Replace %" PRIu64 "\n", block_index);
            break;
    }
}