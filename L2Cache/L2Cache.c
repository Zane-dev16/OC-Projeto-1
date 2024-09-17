#include "L2Cache.h"

uint8_t L1Cache[L1_SIZE];
uint8_t L2Cache[L2_SIZE];
uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache SimpleCacheL1;
Cache SimpleCacheL2;


/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {

    if (address >= DRAM_SIZE - WORD_SIZE + 1)
        exit(-1);

    if (mode == MODE_READ) {
        memcpy(data, &(DRAM[address]), BLOCK_SIZE);
        time += DRAM_READ_TIME;
    }

    if (mode == MODE_WRITE) {
        memcpy(&(DRAM[address]), data, BLOCK_SIZE);
        time += DRAM_WRITE_TIME;
    }
}

/*********************** Caches *************************/

void initCache(Cache *cache) { cache.init = 0; }

uint32_t createBitMask(uint32_t bits) {
    return ((1 << bits) - 1); 
}

uint32_t getNumIndexBits(uint32_t cacheSize) {
    return (uint32_t)log2(cacheSize / BLOCK_SIZE); 
}

uint32_t getNumBlockOffsetBits() {
    return (uint32_t)log2(BLOCK_SIZE); 
}

uint32_t getTag(uint32_t address) {
    return address >> (getNumBlockOffsetBits() + getNumIndexBits(L1_SIZE));
}

uint32_t getL1Index(uint32_t address) {
    return (address >> getNumBlockOffsetBits()) & createBitMask(getNumIndexBits(L1_SIZE));
}

uint32_t getBlockOffset(uint32_t address) {
    return address & createBitMask(getNumBlockOffsetBits());
}

uint32_t getMemAddress(uint32_t address) {
    uint32_t MemAddress = address >> getNumBlockOffsetBits();
    MemAddress = MemAddress << getNumBlockOffsetBits();
    return MemAddress;
}

void accessCache(Cache *cache, uint8_t *cacheData, uint32_t address, uint8_t *data, uint32_t mode, uint32_t cacheSize, uint32_t readTime, uint32_t writeTime) {

    uint32_t index, Tag, MemAddress, BlockOffset, CacheBlockIndex, CacheDataIndex;
    uint8_t TempBlock[BLOCK_SIZE];

    index = getIndex(address, cacheSize);
    Tag = getTag(address, cacheSize);
    MemAddress = getMemAddress(address);
    BlockOffset = getBlockOffset(address);
    CacheBlockIndex = index * BLOCK_SIZE;
    CacheDataIndex = CacheBlockIndex + BlockOffset;

    /* init cache */
    if (cache->init == 0) {
        for (int i = 0; i < cacheSize / BLOCK_SIZE; i++) {
            cache->lines[i].Valid = 0;
        }
        cache->init = 1;
    }

    CacheLine *Line = &cache->lines[index];

    /* access Cachen */
    if (!Line->Valid || Line->Tag != Tag) {             // if block not present - miss
        accessDRAM(MemAddress, TempBlock, MODE_READ);   // get new block from DRAM

        if ((Line->Valid) && (Line->Dirty)) {           // line has dirty block
            MemAddress = Line->Tag << 3;                // get address of the block in memory
            accessDRAM(MemAddress, &(cacheData[CacheBlockIndex]), MODE_WRITE);  // then write back old block
        }

        memcpy(&(cacheData[CacheBlockIndex]), TempBlock,
                BLOCK_SIZE); // copy new block to cache line
        Line->Valid = 1;
        Line->Tag = Tag;
        Line->Dirty = 0;
    } // if miss, then replaced with the correct block

    if (mode == MODE_READ) {    // read data from cache line
        memcpy(data, &(cacheData[CacheDataIndex]), WORD_SIZE);
        time += readTime;
    }

    if (mode == MODE_WRITE) { // write data from cache line
        memcpy(&(cacheData[CacheDataIndex]), data, WORD_SIZE);
        time += writeTime;
        Line->Dirty = 1;
    }
}

void cachesAccessesHandler(uint32_t address, uint8_t *data, uint32_t mode) {

    // access L1
    accessCache(&SimpleCacheL1, L1Cache, address, data, mode, L1_SIZE, L1_READ_TIME, L1_WRITE_TIME);

    // check miss
    if (SimpleCacheL1.lines[getIndex(address, L1_SIZE)].Valid && 
        SimpleCacheL1.lines[getIndex(address, L1_SIZE)].Tag == getTag(address, L1_SIZE)) {
        return; // L1 hit
    }

    // access L2
    accessCache(&SimpleCacheL2, L2Cache, address, data, mode, L2_SIZE, L2_READ_TIME, L2_WRITE_TIME);

    // check hit in L2
    if (SimpleCacheL2.lines[getIndex(address, L2_SIZE)].Valid && 
        SimpleCacheL2.lines[getIndex(address, L2_SIZE)].Tag == getTag(address, L2_SIZE)) {
        // L2 hit -> copy to L1
        uint8_t TempBlock[BLOCK_SIZE];
        memcpy(TempBlock, &(L2Cache[getIndex(address, L2_SIZE) * BLOCK_SIZE]), BLOCK_SIZE);
        accessCache(&SimpleCacheL1, L1Cache, address, TempBlock, MODE_WRITE, L1_SIZE, L1_READ_TIME, L1_WRITE_TIME);
    } else {
        // L2 miss -> copy to L1 and L2
        accessDRAM(address, data, MODE_READ);
        accessCache(&SimpleCacheL1, L1Cache, address, data, MODE_WRITE, L1_SIZE, L1_READ_TIME, L1_WRITE_TIME);
        accessCache(&SimpleCacheL2, L2Cache, address, data, MODE_WRITE, L2_SIZE, L2_READ_TIME, L2_WRITE_TIME);
    }
}

void read(uint32_t address, uint8_t *data) {
    cachesAccessesHandler(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
    cachesAccessesHandler(address, data, MODE_WRITE);
}
