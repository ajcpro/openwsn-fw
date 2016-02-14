#ifndef __OPENQUEUE_H
#define __OPENQUEUE_H

/**
\addtogroup cross-layers
\{
\addtogroup OpenQueue
\{
*/

#include "opendefs.h"
#include "IEEE802154.h"

//=========================== define ==========================================

#define QUEUELENGTH  15

#define BIGQUEUELENGTH  5

//=========================== typedef =========================================

typedef struct {
   uint8_t  creator;
   uint8_t  owner;
} debugOpenQueueEntry_t;

typedef struct {
   bool     in_use;
   uint8_t  buffer[LARGE_PACKET_SIZE];
} BigQueueEntry_t;

//=========================== module variables ================================

typedef struct {
   OpenQueueEntry_t queue[QUEUELENGTH];
} openqueue_vars_t;

typedef struct {
   BigQueueEntry_t queue[BIGQUEUELENGTH];
} bigqueue_vars_t;

//=========================== prototypes ======================================

// admin
void               openqueue_init(void);
bool               debugPrint_queue(void);
// called by any component
OpenQueueEntry_t*  openqueue_getFreePacketBuffer(uint8_t creator);
owerror_t          openqueue_freePacketBuffer(OpenQueueEntry_t* pkt);
void               openqueue_removeAllCreatedBy(uint8_t creator);
void               openqueue_removeAllOwnedBy(uint8_t owner);

OpenQueueEntry_t*  openqueue_toBigPacket(OpenQueueEntry_t* pkt, uint16_t start);
// called by res
OpenQueueEntry_t*  openqueue_sixtopGetSentPacket(void);
OpenQueueEntry_t*  openqueue_sixtopGetReceivedPacket(void);
// called by IEEE80215E
OpenQueueEntry_t*  openqueue_macGetDataPacket(open_addr_t* toNeighbor);
OpenQueueEntry_t*  openqueue_macGetEBPacket(void);

/**
\}
\}
*/

#endif
