#include <vector>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "cachesim.hpp"

using std::vector;

vector <cache_t> cache;
cache_info_t info;
int ctid_init = 1;

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

void update_policy(int cache_ind, uint64_t set_ind, int block_ind) {
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

        /* Check if the mru block is the first in block */
        if (cache[cache_ind].sets[set_ind].blocks[first].mru) {
            /* Take the second block and decrement all above it */
            decrement_start_ind = 2;
        }
        else {
            /* Take the first block and decrement all above it */
            decrement_start_ind = 1;
        }

        /* Go through the blocks and decrement */
        for (int i = 0; i < info.num_blocks_set; i++) {
            if (cache[cache_ind].sets[set_ind].blocks[i].num_fifo >= decrement_start_ind)
                cache[cache_ind].sets[set_ind].blocks[i].num_fifo -= 1;
        }

        /* Set the block we want to the last in */
        cache[cache_ind].sets[set_ind].blocks[decrement_start_ind-1].num_fifo = 
                        info.num_blocks_set - 1;

        /* Return the correct indice */
        if (decrement_start_ind == 1)
            return first;
        else
            return second;

        /* Throw an asser */
        printf("\nREPLACEMENT ALGORITHM FAILED!!!!\n");
        assert(false);

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

        // myfile0 = fopen("output0.txt", "w");
        // myfile1 = fopen("output1.txt", "w");
        // myfile2 = fopen("output2.txt", "w");
        // myfile3 = fopen("output3.txt", "w");
        // myfile4 = fopen("output4.txt", "w");
        // myfile5 = fopen("output5.txt", "w");
        // myfile6 = fopen("output6.txt", "w");
        // myfile7 = fopen("output7.txt", "w");

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
    bool count_it = false;

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

        /* If the set number changes then we should track whether this access
        is a hit or a miss and increment the p_stats struct */
        if (index != old_index) {
            count_it = true;
            old_index = index;
        }
         
        /* Some debug code */
         // if (count_it) {
         //    switch (cache_ind) {
         //        case 0:
         //            fprintf(myfile0, "%c\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%d",
         //                    rw, tag, index, offset, count_it);
         //            break;
         //        case 1:
         //            fprintf(myfile1, "%c\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%d\n",
         //                    rw, tag, index, offset, count_it);
         //            break;
         //        case 2:
         //            fprintf(myfile2, "%c\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%d\n",
         //                    rw, tag, index, offset, count_it);
         //            break;
         //        case 3:
         //            fprintf(myfile3, "%c\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%d\n",
         //                    rw, tag, index, offset, count_it);
         //            break;
         //        case 4:
         //            fprintf(myfile4, "%c\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%d\n",
         //                    rw, tag, index, offset, count_it);
         //            break;
         //        case 5:
         //            fprintf(myfile5, "%c\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%d\n",
         //                    rw, tag, index, offset, count_it);
         //            break;
         //        case 6:
         //            fprintf(myfile6, "%c\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%d\n",
         //                    rw, tag, index, offset, count_it);
         //            break;
         //        case 7:
         //            fprintf(myfile7, "%c\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%d\n",
         //                    rw, tag, index, offset, count_it);
         //            break;

         //    }

         // }

        /* The main cache access function */
        cache_lookup(ctid, cache_ind, tag, index, offset, p_stats, count_it, rw);

        /* Increment the address and set count_it to false */
        address += 1;
        count_it = false;
    }

}

void cache_lookup(int ctid, int cache_ind, uint64_t tag, uint64_t index, 
                 uint64_t offset, cache_stats_t* p_stats, bool count_it, char rw) {

    bool missed = true; /* Assume we miss */

    /* Check if we need half blocks */
    if (info.ST) {

    }
    else { /* Blocking */

        /* Loop through the blocks in the set and see if we have a hit */
        for (int block_ind = 0; block_ind < info.num_blocks_set; block_ind++ ) {
            /* Get the tag and compare */
            if (cache[cache_ind].sets[index].blocks[block_ind].tag == tag) {

                /* Update the replacement policy */
                update_policy(cache_ind, index, block_ind);

                /* Set the block to dirty */
                if (rw == WRITE)
                    cache[cache_ind].sets[index].blocks[block_ind].dirty = true;

                missed = false;
                break;

            }
        }

        if (missed) {

            if (count_it) {
                if (rw == WRITE) {
                    if (ctid == 0) {
                        p_stats->write_misses += 1;
                        //fprintf(myfile0, "Write Miss\n");
                    }
                    p_stats->write_misses_combined += 1;
                }
                else {
                    if (ctid == 0) {
                        p_stats->read_misses += 1;
                        //fprintf(myfile0, "Read Miss\n");
                    }
                    p_stats->read_misses_combined += 1;
                }
            }

            /* Get a replacement block */
            int block_ind = get_replacement_block(cache_ind, index);

            /* Replace the block */
            cache[cache_ind].sets[index].blocks[block_ind].tag = tag;
            cache[cache_ind].sets[index].blocks[block_ind].valid = true;
            if (rw == WRITE)
                cache[cache_ind].sets[index].blocks[block_ind].dirty = true;
            else
                cache[cache_ind].sets[index].blocks[block_ind].dirty = false;

            /* Update the replacement policy */
            update_policy(cache_ind, index, block_ind);
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

    // for (int i = 0; i < info.num_processors; i++) {
    //     printf("Cache[%d]\n", i);
    //     for (int j = 0; j < info.num_sets; j++) {
    //         printf("\tSet[%d]\n", j);
    //         for (int k = 0; k < info.num_blocks_set; k++) {
    //             printf("\t\tBlock[%d]\n", k);
    //             printf("\t\t\tTag: %" PRIu64 "\n", cache[i].sets[j].blocks[k].tag);
    //             if (cache[i].sets[j].blocks[k].dirty)
    //                 printf("\t\t\tDirty: true\n");
    //             else
    //                 printf("\t\t\tDirty: false\n");

    //             if (cache[i].sets[j].blocks[k].valid)
    //                 printf("\t\t\tValid: true\n");
    //             else
    //                 printf("\t\t\tValid: false\n");

    //             if (cache[i].sets[j].blocks[k].valid_first)
    //                 printf("\t\t\tValid_First: true\n");
    //             else
    //                 printf("\t\t\tValid_First: false\n");

    //             if (cache[i].sets[j].blocks[k].valid_first)
    //                 printf("\t\t\tValid_Second: true\n");
    //             else
    //                 printf("\t\t\tValid_Second: false\n");

    //             printf("\t\t\tLRU: %d\n", cache[i].sets[j].blocks[k].num_lru);
    //             printf("\t\t\tFIFO: %d\n", cache[i].sets[j].blocks[k].num_fifo);
    //             if (cache[i].sets[j].blocks[k].mru)
    //                 printf("\t\t\tMRU: true\n");
    //             else
    //                 printf("\t\t\tMRU: false\n");

    //         }
    //     }
    // }

    for (int i = 0; i < info.num_processors; i++) {
        for (int j = 0; j < info.num_sets; j++) {
            cache[i].sets[j].blocks.clear();
        }
        cache[i].sets.clear();
    }

    // fclose(myfile0);
    // fclose(myfile1);
    // fclose(myfile2);
    // fclose(myfile3);
    // fclose(myfile4);
    // fclose(myfile5);
    // fclose(myfile6);
    // fclose(myfile7);

    cache.clear();

}
