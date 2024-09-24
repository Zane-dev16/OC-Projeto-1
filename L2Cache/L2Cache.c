#include "L2Cache.h"

uint8_t L1Cache[L1_SIZE];
uint8_t L2Cache[L2_SIZE];
uint8_t DRAM[DRAM_SIZE];
uint32_t time;
CacheL1 SimpleCacheL1;
CacheL2 SimpleCacheL2;


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

void initCaches() {
    SimpleCacheL1.init = 0;
    SimpleCacheL2.init = 0;
}

uint32_t createBitMask(uint32_t bits) {
    return ((1 << bits) - 1); 
}

uint32_t getNumIndexBits(uint32_t cacheSize) {
    return (uint32_t)log2(cacheSize / BLOCK_SIZE); 
}

uint32_t getNumBlockOffsetBits() {
    return (uint32_t)log2(BLOCK_SIZE); 
}

uint32_t getTag(uint32_t address, uint32_t cacheSize) {
    return address >> (getNumBlockOffsetBits() + getNumIndexBits(cacheSize));
}

uint32_t getIndex(uint32_t address, uint32_t cacheSize) {
    return (address >> getNumBlockOffsetBits()) & createBitMask(getNumIndexBits(cacheSize));
}

uint32_t getBlockOffset(uint32_t address) {
    return address & createBitMask(getNumBlockOffsetBits());
}

uint32_t getMemAddress(uint32_t address) {
    uint32_t MemAddress = address >> getNumBlockOffsetBits();
    MemAddress = MemAddress << getNumBlockOffsetBits();
    return MemAddress;
}

uint32_t getMemAddressFromCacheInfo(uint32_t Tag, uint32_t index, uint32_t cacheSize) {
    uint32_t MemAddress;
    MemAddress = Tag << getNumIndexBits(cacheSize);        
    MemAddress = MemAddress | index;
    MemAddress = MemAddress << getNumBlockOffsetBits();
    return MemAddress; 
}

/*********************** Cache L2 *************************/

void accessL2Cache(uint32_t address, uint8_t *data, uint32_t mode) {

    uint32_t index, Tag, MemAddress, BlockOffset, CacheBlockIndex, CacheDataIndex;
    uint8_t TempBlock[BLOCK_SIZE];

    index = getIndex(address, L2_SIZE);
    Tag = getTag(address, L2_SIZE);
    MemAddress = getMemAddress(address);
    BlockOffset = getBlockOffset(address);
    CacheBlockIndex = index * BLOCK_SIZE;
    CacheDataIndex = CacheBlockIndex + BlockOffset;

    /* init cache */
    if (SimpleCacheL2.init == 0) {
        for (int i = 0; i < L2_SIZE / BLOCK_SIZE; i++) {
            SimpleCacheL2.lines[i].Valid = 0;
        }
        SimpleCacheL2.init = 1;
    }

    CacheLine *Line = &SimpleCacheL2.lines[index];

    /* access Cachen */
    if (!Line->Valid || Line->Tag != Tag) {             // if block not present - miss
        accessDRAM(MemAddress, TempBlock, MODE_READ);   // get new block from DRAM

        if ((Line->Valid) && (Line->Dirty)) {           // line has dirty block
            MemAddress = getMemAddressFromCacheInfo(Line->Tag, index, L2_SIZE);
            accessDRAM(MemAddress, &(L2Cache[CacheBlockIndex]), MODE_WRITE); // then write back old block
        }

        memcpy(&(L2Cache[CacheBlockIndex]), TempBlock, BLOCK_SIZE); // copy new block to L2
        Line->Valid = 1;
        Line->Tag = Tag;
        Line->Dirty = 0;
    }

    if (mode == MODE_READ) {    // read data from cache line
        memcpy(data, &(L2Cache[CacheDataIndex]), WORD_SIZE);
        time += L2_READ_TIME;
    }

    if (mode == MODE_WRITE) { // write data to cache
        memcpy(&(L2Cache[CacheDataIndex]), data, WORD_SIZE);
        time += L2_WRITE_TIME;
        Line->Dirty = 1;
    }
}

/*********************** Cache L1 *************************/

void accessL1Cache(uint32_t address, uint8_t *data, uint32_t mode) {

    uint32_t index, Tag, MemAddress, BlockOffset, CacheBlockIndex, CacheDataIndex;
    uint8_t TempBlock[BLOCK_SIZE];

    index = getIndex(address, L1_SIZE);
    Tag = getTag(address, L1_SIZE);
    MemAddress = getMemAddress(address);
    BlockOffset = getBlockOffset(address);
    CacheBlockIndex = index * BLOCK_SIZE;
    CacheDataIndex = CacheBlockIndex + BlockOffset;

    /* init cache */
    if (SimpleCacheL1.init == 0) {
        for (int i = 0; i < L1_SIZE / BLOCK_SIZE; i++) {
            SimpleCacheL1.lines[i].Valid = 0;
        }
        SimpleCacheL1.init = 1;
    }

    CacheLine *Line = &SimpleCacheL1.lines[index];

    /* access Cachen */
    if (!Line->Valid || Line->Tag != Tag) {             // if block not present - miss
        accessL2Cache(address, TempBlock, MODE_READ);   // get new block from L2

        if ((Line->Valid) && (Line->Dirty)) {           // line has dirty block
            MemAddress = getMemAddressFromCacheInfo(Line->Tag, index, L1_SIZE);
            accessL2Cache(MemAddress, &(L1Cache[CacheBlockIndex]), MODE_WRITE); // then write back old block
        }

        memcpy(&(L1Cache[CacheBlockIndex]), TempBlock, BLOCK_SIZE); // copy new block to L1
        Line->Valid = 1;
        Line->Tag = Tag;
        Line->Dirty = 0;
    }

    if (mode == MODE_READ) {    // read data from cache line
        memcpy(data, &(L1Cache[CacheDataIndex]), WORD_SIZE);
        time += L1_READ_TIME;
    }

    if (mode == MODE_WRITE) { // write data to cache
        memcpy(&(L1Cache[CacheDataIndex]), data, WORD_SIZE);
        time += L1_WRITE_TIME;
        Line->Dirty = 1;
    }
}

void read(uint32_t address, uint8_t *data) {
    accessL1Cache(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
    accessL1Cache(address, data, MODE_WRITE);
}
