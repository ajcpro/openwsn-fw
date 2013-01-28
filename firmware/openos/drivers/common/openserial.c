/**
\brief Definition of the "openserial" driver.

\author Fabien Chraim <chraim@eecs.berkeley.edu>, March 2012.
*/

#include "openwsn.h"
#include "openserial.h"
#include "IEEE802154E.h"
#include "neighbors.h"
#include "res.h"
#include "icmpv6echo.h"
#include "icmpv6router.h"
#include "idmanager.h"
#include "openqueue.h"
#include "tcpinject.h"
#include "udpinject.h"
#include "openbridge.h"
#include "leds.h"
#include "schedule.h"
#include "uart.h"
#include "opentimers.h"
#include "serialecho.h"
#include "hdlcserial.h"

//=========================== variables =======================================

typedef struct {
   // admin
   uint8_t    mode;
   uint8_t    debugPrintCounter;
   // input
   uint8_t    reqFrame[1+1+2+1]; // flag (1B), command (2B), CRC (1B), flag (1B)
   uint8_t    reqFrameIdx;
   bool       busyReceiving;
   uint16_t   inputBufFill;
   uint8_t    inputBuf[SERIAL_INPUT_BUFFER_SIZE];
   // output
   bool       outputBufFilled;
   uint16_t   crc;
   uint8_t    outputBufIdxW;
   uint8_t    outputBufIdxR;
   uint8_t    outputBuf[SERIAL_OUTPUT_BUFFER_SIZE];
} openserial_vars_t;

openserial_vars_t openserial_vars;

//=========================== prototypes ======================================

uint8_t IncrOutputBufIdxW();
uint8_t IncrOutputBufIdxR();
error_t openserial_printInfoErrorCritical(
   char             severity,
   uint8_t          calling_component,
   uint8_t          error_code,
   errorparameter_t arg1,
   errorparameter_t arg2
);

//=========================== public ==========================================

void openserial_init() {
   // reset variable
   memset(&openserial_vars,0,sizeof(openserial_vars_t));
   
   // admin
   openserial_vars.mode                = MODE_OFF;
   openserial_vars.debugPrintCounter   = 0;
   
   // input
   openserial_vars.reqFrame[0]         = SERFRAME_MOTE2PC_REQUEST;
   hdlcify(openserial_vars.reqFrame,1);
   openserial_vars.reqFrameIdx         = 0;
   openserial_vars.busyReceiving       = FALSE;
   openserial_vars.inputBufFill        = 0;
   
   // ouput
   openserial_vars.outputBufFilled     = FALSE;
   openserial_vars.outputBufIdxR       = 0;
   openserial_vars.outputBufIdxW       = 0;
   
   // set callbacks
   uart_setCallbacks(isr_openserial_tx,
                     isr_openserial_rx);
}

//########################### poipoipoipoi ####################################
void hdlcOpen() {
   openserial_vars.crc                                = 0xffff;
   openserial_vars.outputBuf[IncrOutputBufIdxW()]     = HDLC_FLAG;
}
void hdlcWrite(uint8_t b) {
   openserial_vars.outputBuf[IncrOutputBufIdxW()]     = b;
}
void hdlcClose() {
   openserial_vars.crc                                = 0x0000;// poipoipoi
   openserial_vars.outputBuf[IncrOutputBufIdxW()]     = (openserial_vars.crc>>0)&0xff;
   openserial_vars.outputBuf[IncrOutputBufIdxW()]     = (openserial_vars.crc>>8)&0xff;
   openserial_vars.outputBuf[IncrOutputBufIdxW()]     = HDLC_FLAG;
}
//########################### poipoipoipoi ####################################

error_t openserial_printStatus(uint8_t statusElement,uint8_t* buffer, uint16_t length) {
   uint8_t i;
   INTERRUPT_DECLARATION();
   
   DISABLE_INTERRUPTS();
   openserial_vars.outputBufFilled  = TRUE;
   hdlcOpen();
   hdlcWrite(SERFRAME_MOTE2PC_STATUS);
   hdlcWrite(idmanager_getMyID(ADDR_16B)->addr_16b[0]);
   hdlcWrite(idmanager_getMyID(ADDR_16B)->addr_16b[1]);
   hdlcWrite(statusElement);
   for (i=0;i<length;i++){
      hdlcWrite(buffer[i]);
   }
   hdlcClose();
   ENABLE_INTERRUPTS();
   
   return E_SUCCESS;
}

error_t openserial_printInfoErrorCritical(
      char             severity,
      uint8_t          calling_component,
      uint8_t          error_code,
      errorparameter_t arg1,
      errorparameter_t arg2
   ) {
   INTERRUPT_DECLARATION();
   
   DISABLE_INTERRUPTS();
   openserial_vars.outputBufFilled  = TRUE;
   hdlcOpen();
   hdlcWrite(severity);
   hdlcWrite(idmanager_getMyID(ADDR_16B)->addr_16b[0]);
   hdlcWrite(idmanager_getMyID(ADDR_16B)->addr_16b[1]);
   hdlcWrite(calling_component);
   hdlcWrite(error_code);
   hdlcWrite((uint8_t)((arg1 & 0xff00)>>8));
   hdlcWrite((uint8_t) (arg1 & 0x00ff));
   hdlcWrite((uint8_t)((arg2 & 0xff00)>>8));
   hdlcWrite((uint8_t) (arg2 & 0x00ff));
   hdlcClose();
   ENABLE_INTERRUPTS();
   
   return E_SUCCESS;
}

error_t openserial_printData(uint8_t* buffer, uint8_t length) {
   uint8_t  i;
   uint8_t  asn[5];
   INTERRUPT_DECLARATION();
   
   // retrieve ASN
   asnWriteToSerial(asn);// byte01,byte23,byte4
   
   DISABLE_INTERRUPTS();
   openserial_vars.outputBufFilled  = TRUE;
   hdlcOpen();
   hdlcWrite(SERFRAME_MOTE2PC_DATA);
   hdlcWrite(idmanager_getMyID(ADDR_16B)->addr_16b[1]);
   hdlcWrite(idmanager_getMyID(ADDR_16B)->addr_16b[0]);
   hdlcWrite(asn[0]);
   hdlcWrite(asn[1]);
   hdlcWrite(asn[2]);
   hdlcWrite(asn[3]);
   hdlcWrite(asn[4]);
   for (i=0;i<length;i++){
      hdlcWrite(buffer[i]);
   }
   hdlcClose();
   ENABLE_INTERRUPTS();
   
   return E_SUCCESS;
}

error_t openserial_printInfo(uint8_t calling_component, uint8_t error_code,
                              errorparameter_t arg1,
                              errorparameter_t arg2) {
   return openserial_printInfoErrorCritical(
      SERFRAME_MOTE2PC_INFO,
      calling_component,
      error_code,
      arg1,
      arg2
   );
}

error_t openserial_printError(uint8_t calling_component, uint8_t error_code,
                              errorparameter_t arg1,
                              errorparameter_t arg2) {
   // blink error LED, this is serious
   leds_error_toggle();
   
   return openserial_printInfoErrorCritical(
      SERFRAME_MOTE2PC_ERROR,
      calling_component,
      error_code,
      arg1,
      arg2
   );
}

error_t openserial_printCritical(uint8_t calling_component, uint8_t error_code,
                              errorparameter_t arg1,
                              errorparameter_t arg2) {
   // blink error LED, this is serious
   leds_error_blink();
   
   // schedule for the mote to reboot in 10s
   opentimers_start(10000,
                    TIMER_ONESHOT,TIME_MS,
                    board_reset);
   
   return openserial_printInfoErrorCritical(
      SERFRAME_MOTE2PC_CRITICAL,
      calling_component,
      error_code,
      arg1,
      arg2
   );
}

uint8_t openserial_getNumDataBytes() {
   uint16_t inputBufFill;
   INTERRUPT_DECLARATION();
   
   DISABLE_INTERRUPTS();
   inputBufFill = openserial_vars.inputBufFill;
   ENABLE_INTERRUPTS();

   return inputBufFill-1; // removing the command byte
}

uint8_t openserial_getInputBuffer(uint8_t* bufferToWrite, uint8_t maxNumBytes) {
   uint8_t  numBytesWritten;
   uint16_t inputBufFill;
   INTERRUPT_DECLARATION();
   
   DISABLE_INTERRUPTS();
   inputBufFill = openserial_vars.inputBufFill;
   ENABLE_INTERRUPTS();
   
   if (maxNumBytes<inputBufFill-1) {
      openserial_printError(COMPONENT_OPENSERIAL,ERR_GETDATA_ASKS_TOO_FEW_BYTES,
                            (errorparameter_t)maxNumBytes,
                            (errorparameter_t)inputBufFill-1);
      numBytesWritten = 0;
   } else {
      numBytesWritten = inputBufFill-1;
      memcpy(bufferToWrite,&(openserial_vars.inputBuf[1]),numBytesWritten);
   }
   DISABLE_INTERRUPTS();
   openserial_vars.inputBufFill=0;
   ENABLE_INTERRUPTS();
   
   return numBytesWritten;
}

void openserial_startInput() {
   INTERRUPT_DECLARATION();
   
   if (openserial_vars.inputBufFill>0) {
      openserial_printError(COMPONENT_OPENSERIAL,ERR_INPUTBUFFER_LENGTH,
                            (errorparameter_t)openserial_vars.inputBufFill,
                            (errorparameter_t)0);
      openserial_vars.inputBufFill = 0;
   }
   
   uart_clearTxInterrupts();
   uart_clearRxInterrupts();      // clear possible pending interrupts
   uart_enableInterrupts();       // Enable USCI_A1 TX & RX interrupt
   
   DISABLE_INTERRUPTS();
   openserial_vars.mode           = MODE_INPUT;
   openserial_vars.reqFrameIdx    = 0;
   uart_writeByte(openserial_vars.reqFrame[openserial_vars.reqFrameIdx]);
   ENABLE_INTERRUPTS();
}

void openserial_startOutput() {
   //schedule a task to get new status in the output buffer
   uint8_t debugPrintCounter;
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   openserial_vars.debugPrintCounter = (openserial_vars.debugPrintCounter+1)%STATUS_MAX;
   debugPrintCounter = openserial_vars.debugPrintCounter;
   ENABLE_INTERRUPTS();
   
   // print debug information
   switch (debugPrintCounter) {
      case STATUS_ISSYNC:
         if (debugPrint_isSync()==TRUE) {
            break;
         }
      case STATUS_ID:
         if (debugPrint_id()==TRUE) {
            break;
         }
      case STATUS_DAGRANK:
         if (debugPrint_myDAGrank()==TRUE) {
            break;
         }
      case STATUS_OUTBUFFERINDEXES:
         if (debugPrint_outBufferIndexes()==TRUE) {
            break;
         }
      case STATUS_ASN:
         if (debugPrint_asn()==TRUE) {
            break;
         }
      case STATUS_MACSTATS:
         if (debugPrint_macStats()==TRUE) {
            break;
         }
      case STATUS_SCHEDULE:
         if(debugPrint_schedule()==TRUE) {
            break;
         }
      case STATUS_BACKOFF:
         if(debugPrint_backoff()==TRUE) {
            break;
         }
      case STATUS_QUEUE:
         if(debugPrint_queue()==TRUE) {
            break;
         }
      case STATUS_NEIGHBORS:
         if (debugPrint_neighbors()==TRUE) {
            break;
         }
      default:
         DISABLE_INTERRUPTS();
         openserial_vars.debugPrintCounter=0;
         ENABLE_INTERRUPTS();
   }
   
   // flush buffer
   uart_clearTxInterrupts();
   uart_clearRxInterrupts();          // clear possible pending interrupts
   uart_enableInterrupts();           // Enable USCI_A1 TX & RX interrupt
   DISABLE_INTERRUPTS();
   openserial_vars.mode=MODE_OUTPUT;
   if (openserial_vars.outputBufFilled) {
      openserial_vars.outputBufIdxR    = 0;//poipoipoi
      uart_writeByte(openserial_vars.outputBuf[IncrOutputBufIdxR()]);
   } else {
      openserial_stop();
   }
   ENABLE_INTERRUPTS();
}

void openserial_stop() {
   uint16_t  inputBufFill;
   uint8_t   cmdByte;
   INTERRUPT_DECLARATION();
   
   DISABLE_INTERRUPTS();
   inputBufFill = openserial_vars.inputBufFill;
   ENABLE_INTERRUPTS();
   
   // disable USCI_A1 TX & RX interrupt
   uart_disableInterrupts();
   
   DISABLE_INTERRUPTS();
   openserial_vars.mode=MODE_OFF;
   ENABLE_INTERRUPTS();
   
   if (inputBufFill>0) {
      DISABLE_INTERRUPTS();
      cmdByte = openserial_vars.inputBuf[0];
      ENABLE_INTERRUPTS();
      switch (cmdByte) {
         case SERFRAME_PC2MOTE_SETROOT:
            idmanager_triggerAboutRoot();
            break;
         case SERFRAME_PC2MOTE_SETBRIDGE:
            idmanager_triggerAboutBridge();
            break;
         case SERFRAME_PC2MOTE_DATA:
            openbridge_triggerData();
            break;
         case SERFRAME_PC2MOTE_TRIGGERTCPINJECT:
            tcpinject_trigger();
            break;
         case SERFRAME_PC2MOTE_TRIGGERUDPINJECT:
            udpinject_trigger();
            break;
         case SERFRAME_PC2MOTE_TRIGGERICMPv6ECHO:
            icmpv6echo_trigger();
            break;
         case SERFRAME_PC2MOTE_TRIGGERSERIALECHO:
            serialecho_echo(&openserial_vars.inputBuf[1],inputBufFill-1);
            break;   
         default:
            openserial_printError(COMPONENT_OPENSERIAL,ERR_UNSUPPORTED_COMMAND,
                                  (errorparameter_t)cmdByte,
                                  (errorparameter_t)0);
            DISABLE_INTERRUPTS();
            openserial_vars.inputBufFill = 0;
            ENABLE_INTERRUPTS();
            break;
      }
   }
}

/**
\brief Trigger this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_outBufferIndexes() {
   uint16_t temp_buffer[2];
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   temp_buffer[0] = openserial_vars.outputBufIdxW;
   temp_buffer[1] = openserial_vars.outputBufIdxR;
   ENABLE_INTERRUPTS();
   openserial_printStatus(STATUS_OUTBUFFERINDEXES,(uint8_t*)temp_buffer,sizeof(temp_buffer));
   return TRUE;
}

//=========================== private =========================================

/**
\pre Disable interrupts before calling this function.
*/
uint8_t IncrOutputBufIdxW() {
   openserial_vars.outputBufIdxW = (openserial_vars.outputBufIdxW+1)%SERIAL_OUTPUT_BUFFER_SIZE;
   return openserial_vars.outputBufIdxW;
}

/**
\pre Disable interrupts before calling this function.
*/
uint8_t IncrOutputBufIdxR() {
   openserial_vars.outputBufIdxR = (openserial_vars.outputBufIdxR+1)%SERIAL_OUTPUT_BUFFER_SIZE;
   return openserial_vars.outputBufIdxR;
}

//=========================== interrupt handlers ==============================

//executed in ISR, called from scheduler.c
void isr_openserial_tx() {
   switch (openserial_vars.mode) {
      case MODE_INPUT:
         openserial_vars.reqFrameIdx++;
         if (openserial_vars.reqFrameIdx<sizeof(openserial_vars.reqFrame)) {
            uart_writeByte(openserial_vars.reqFrame[openserial_vars.reqFrameIdx]);
         }
         break;
      case MODE_OUTPUT:
         if (openserial_vars.outputBufIdxW==openserial_vars.outputBufIdxR) {
            openserial_vars.outputBufFilled = FALSE;
            openserial_vars.outputBufIdxR   = 0;//poipoi
            openserial_vars.outputBufIdxW   = 0;//poipoi
         }
         if (openserial_vars.outputBufFilled) {
            uart_writeByte(openserial_vars.outputBuf[IncrOutputBufIdxR()]);
         }
         break;
      case MODE_OFF:
      default:
         break;
   }
}

// executed in ISR, called from scheduler.c
void isr_openserial_rx() {
   uint8_t rxbyte;
   
   // stop if I'm not in input mode
   if (openserial_vars.mode!=MODE_INPUT) {
      return;
   }
   
   // read byte just received
   rxbyte = uart_readByte();
   
   if        (openserial_vars.busyReceiving==FALSE && rxbyte==HDLC_FLAG) {
      // start of frame
      
      openserial_vars.busyReceiving         = TRUE;
      openserial_vars.inputBuf[openserial_vars.inputBufFill++]=rxbyte;
   
   } else if (openserial_vars.busyReceiving==TRUE  && rxbyte!=HDLC_FLAG) {
      // middle of frame
      
      openserial_vars.inputBuf[openserial_vars.inputBufFill++]=rxbyte;
      
      if (openserial_vars.inputBufFill+1>SERIAL_INPUT_BUFFER_SIZE){
         // input buffer overflow
         openserial_printError(COMPONENT_OPENSERIAL,ERR_INPUT_BUFFER_OVERFLOW,
                               (errorparameter_t)0,
                               (errorparameter_t)0);
         openserial_vars.busyReceiving      = FALSE;
         openserial_vars.inputBufFill       = 0;
         openserial_stop();
      }
   } else if (openserial_vars.busyReceiving==TRUE  && rxbyte==HDLC_FLAG) {
         // end of frame
         
         openserial_vars.inputBuf[openserial_vars.inputBufFill++]=rxbyte;
         openserial_vars.inputBufFill = dehdlcify(openserial_vars.inputBuf,openserial_vars.inputBufFill);
         
         if (openserial_vars.inputBufFill==0){
            openserial_printError(COMPONENT_OPENSERIAL,ERR_INPUTBUFFER_BAD_CRC,
                                  (errorparameter_t)0,
                                  (errorparameter_t)0);
         
         }
         
         openserial_vars.busyReceiving             = FALSE;
         openserial_stop();
   }
}