#ifndef SIMPLECACHE_H
#define SIMPLECACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "Cache.h"

void resetTime();

uint32_t getTime();

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t, uint8_t *, uint32_t);

/*********************** Cache *************************/

void initCaches();

uint32_t createBitMask(uint32_t);

uint32_t getNumIndexBits(uint32_t);

uint32_t getNumBlockOffsetBits();

uint32_t getTag(uint32_t, uint32_t);

uint32_t getIndex(uint32_t, uint32_t);

uint32_t getBlockOffset(uint32_t);

uint32_t getMemAddress(uint32_t);

uint32_t getMemAddressFromCacheInfo(uint32_t, uint32_t, uint32_t);

void accessL2Cache(uint32_t, uint8_t *, uint32_t);

void accessL1Cache(uint32_t, uint8_t *, uint32_t);

typedef struct CacheLine {
  uint8_t Valid;
  uint8_t Dirty;
  uint32_t Tag;
} CacheLine;


typedef struct CacheL1 {
  uint32_t init;
  CacheLine lines[L1_SIZE / BLOCK_SIZE];
} CacheL1;

typedef struct CacheL2 {
  uint32_t init;
  CacheLine lines[L2_SIZE / BLOCK_SIZE];
} CacheL2;

/*********************** Interfaces *************************/

void read(uint32_t, uint8_t *);

void write(uint32_t, uint8_t *);

#endif
