#ifndef CACHESIM_HPP
#define CACHESIM_HPP

#include <cinttypes>
#include <vector>

using std::vector;

struct cache_stats_t {
    uint64_t accesses;
    uint64_t reads;
    uint64_t read_misses;
    uint64_t read_misses_combined;
    uint64_t writes;
    uint64_t write_misses;
    uint64_t write_misses_combined;
    uint64_t misses;
    uint64_t hit_time;
    uint64_t miss_penalty;
    double   miss_rate;
    double   avg_access_time;
    uint64_t storage_overhead;
    double   storage_overhead_ratio;
};

typedef uint64_t address_t; /* An address */

uint64_t parse_address(address_t address, char part);
void setup_cache(uint64_t c, uint64_t b, uint64_t s, char st, char r);
void cache_access(unsigned int ctid, char rw, char numOfBytes, 
                    uint64_t address, cache_stats_t* p_stats);
void cache_write(int ctid, int cache_index, uint64_t tag, uint64_t index, 
                uint64_t offset, int numOfBytes, cache_stats_t* p_stats);
void cache_read(int ctid, int cache_index, uint64_t tag, uint64_t index, 
                uint64_t offset, int numOfBytes, cache_stats_t* p_stats);
void complete_cache(cache_stats_t *p_stats);
void update_policy(int cache_index, uint64_t set_ind, int block_ind);
int get_replacement_block(int cache_index, uint64_t set_ind);

static const uint64_t DEFAULT_C = 15;   /* 32KB Cache */
static const uint64_t DEFAULT_B = 5;    /* 32-byte blocks */
static const uint64_t DEFAULT_S = 3;    /* 8 blocks per set */

static const char     BLOCKING = 'B';
static const char     SUBBLOCKING = 'S';
static const char     DEFAULT_ST = BLOCKING;

static const char     LRU = 'L';
static const char     NMRU_FIFO = 'N';
static const char     DEFAULT_R = LRU;

/** Argument to cache_access rw. Indicates a load */
static const char     READ = 'r';
/** Argument to cache_acce
ss rw. Indicates a store */
static const char     WRITE = 'w';



/** My own additions */

static const int ADDRESS_SIZE = 64; /* number of bits for an address*/

/* Info for the cache */
struct cache_info_t {
    int num_processors;
    uint64_t total_bytes;
    uint64_t bytes_per_block;
    uint64_t num_blocks_set;
    uint64_t num_sets;
    uint64_t tag_bits;
    uint64_t index_bits;
    uint64_t offset_bits;
    uint64_t S;
    uint64_t B;
    int C;
    bool ST;
    bool R;
};

/* Overhead */
static const int blocking_bits = 1;
static const int subblocking_bits = 2;
static const int lru_bits = 8;
static const int nmru_fifo_bits = 2;

/* Cache block data structure */
struct cache_block_t {
    address_t tag;
    bool dirty;
    bool valid;
    bool valid_first;
    bool valid_second;
    int num_lru;
    int num_fifo;
    int mru;
};

struct cache_set_t {
    vector <cache_block_t> blocks;
};

struct cache_t {
    vector <cache_set_t> sets;
    unsigned int ctid;
};

#endif /* CACHESIM_HPP */
