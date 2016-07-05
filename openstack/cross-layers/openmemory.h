/**
 * \brief Declaration of the "openmemory" interface.
 *
 * \author Antonio Cepero <cpro@uoc.edu>, March 2016.
*/

#ifndef __OPENMEMORY_H
#define __OPENMEMORY_H

/**
\addtogroup cross-layers
\{
\addtogroup OpenMemory
\{
*/

#include "opendefs.h"

//=========================== define ==========================================

#ifdef ARCH_TOTAL_OPENMEMORY
#undef   TOTAL_DYNAMIC_MEMORY
#define  TOTAL_DYNAMIC_MEMORY ARCH_TOTAL_OPENMEMORY
#endif

/*
  TOTAL_DYNAMIC_MEMORY is the max amount of memory to use and it is part of
  .DATA, not .HEAP. Then, if:
  TOTAL_BLOCKS = TOTAL_DYNAMIC_MEMORY/FRAME_DATA_TOTAL => Z = X/Y
  TOTAL_MEMORY = TOTAL_BLOCKS * FRAME_DATA_TOTAL
  I want that:
  TOTAL_MEMORY_USED = TOTAL_MEMORY + TOTAL_BLOCKS + 1 <= TOTAL_DYNAMIC_MEMORY
  i.e, memory for data vector plus memory for map vector plus the counter
  must be less or equal than TOTAL_DYNAMIC_MEMORY:
       
  (X/Y)*Y + X/Y + 1 <= X
  (X/Y)*(Y+1) + 1 <= X   <=>  Z*(Y+1) <= X-1   <=>  z = (X-1)/(Y+1)
*/
#define FRAME_DATA_BLOCKS ((TOTAL_DYNAMIC_MEMORY-1)/(FRAME_DATA_TOTAL+1))
#if FRAME_DATA_BLOCKS > 254
#undef  FRAME_DATA_BLOCKS
#define FRAME_DATA_BLOCKS 254
#endif

#define TOTAL_MEMORY_SIZE (FRAME_DATA_BLOCKS*FRAME_DATA_TOTAL)

//=========================== typedef =========================================

typedef struct {
   uint8_t  buffer[TOTAL_MEMORY_SIZE];
   uint8_t  map[FRAME_DATA_BLOCKS];
} OpenMemoryEntry_t;

//=========================== module variables ================================

typedef struct {
   OpenMemoryEntry_t memory;
   uint8_t           used;
} openmemory_vars_t;

//=========================== prototypes ======================================

// admin
void      openmemory_init(void);
// called by any component
uint8_t*  openmemory_getMemory(uint16_t size);
owerror_t openmemory_freeMemory(uint8_t* address);
uint8_t*  openmemory_increaseMemory(uint8_t* address, uint16_t size);
uint8_t*  openmemory_firstSegmentAddr(uint8_t* address);
uint8_t*  openmemory_lastSegmentAddr(uint8_t* address);
bool      openmemory_sameMemoryArea(uint8_t* addr1, uint8_t* addr2);

/**
\}
\}
*/

#endif
