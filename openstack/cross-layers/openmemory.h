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

#define FRAME_DATA_BLOCKS (TOTAL_DYNAMIC_MEMORY/FRAME_DATA_TOTAL)

//=========================== typedef =========================================

typedef struct {
   uint8_t  buffer[TOTAL_DYNAMIC_MEMORY];
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
