
#ifndef CO_DRIVER_TARGET_H_
#define CO_DRIVER_TARGET_H_

/* This file contains device and application specific definitions.
 * It is included from CO_driver.h, which contains documentation
 * for definitions below. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef CO_DRIVER_CUSTOM
#include "CO_driver_custom.h"
#endif

#include "driver/can.h"
#include "base/byteorder.h"

#ifdef CONFIG_CO_MULTITHREAD
#include <rtems/thread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Stack configuration override default values.
 * For more information see file CO_config.h. */
#ifndef CO_CONFIG_NMT
#define CO_CONFIG_NMT (CO_CONFIG_NMT_CALLBACK_CHANGE | \
                       CO_CONFIG_NMT_MASTER | \
                       CO_CONFIG_FLAG_CALLBACK_PRE | \
                       CO_CONFIG_FLAG_TIMERNEXT)
#endif

#ifndef CO_CONFIG_HB_CONS
#define CO_CONFIG_HB_CONS (CO_CONFIG_HB_CONS_ENABLE | \
                           CO_CONFIG_HB_CONS_CALLBACK_MULTI | \
                           CO_CONFIG_HB_CONS_QUERY_FUNCT | \
                           CO_CONFIG_FLAG_CALLBACK_PRE | \
                           CO_CONFIG_FLAG_TIMERNEXT)
#endif

#ifndef CO_CONFIG_EM
#define CO_CONFIG_EM (CO_CONFIG_EM_PRODUCER | \
                      CO_CONFIG_EM_HISTORY | \
                      CO_CONFIG_EM_CONSUMER | \
                      CO_CONFIG_FLAG_CALLBACK_PRE | \
                      CO_CONFIG_FLAG_TIMERNEXT | \
                      CO_CONFIG_FLAG_OD_DYNAMIC)
#endif

#ifndef CO_CONFIG_SDO_SRV
#define CO_CONFIG_SDO_SRV (CO_CONFIG_SDO_SRV_SEGMENTED | \
                           CO_CONFIG_SDO_SRV_BLOCK | \
                           CO_CONFIG_FLAG_CALLBACK_PRE | \
                           CO_CONFIG_FLAG_TIMERNEXT | \
                           CO_CONFIG_FLAG_OD_DYNAMIC)
#endif

#ifndef CO_CONFIG_SDO_SRV_BUFFER_SIZE
#define CO_CONFIG_SDO_SRV_BUFFER_SIZE (127*7)
#endif

#ifndef CO_CONFIG_SDO_CLI
#define CO_CONFIG_SDO_CLI (CO_CONFIG_SDO_CLI_ENABLE | \
                           CO_CONFIG_SDO_CLI_SEGMENTED | \
                           CO_CONFIG_SDO_CLI_BLOCK | \
                           CO_CONFIG_SDO_CLI_LOCAL | \
                           CO_CONFIG_FLAG_CALLBACK_PRE | \
                           CO_CONFIG_FLAG_TIMERNEXT | \
                           CO_CONFIG_FLAG_OD_DYNAMIC)
#endif

#ifndef CO_CONFIG_SDO_CLI_BUFFER_SIZE
#define CO_CONFIG_SDO_CLI_BUFFER_SIZE 1000
#endif

#ifndef CO_CONFIG_TIME
#define CO_CONFIG_TIME (CO_CONFIG_TIME_ENABLE | \
                        CO_CONFIG_TIME_PRODUCER | \
                        CO_CONFIG_FLAG_CALLBACK_PRE)
#endif

#ifndef CO_CONFIG_SYNC
#define CO_CONFIG_SYNC (CO_CONFIG_SYNC_ENABLE | \
                        CO_CONFIG_SYNC_PRODUCER | \
                        CO_CONFIG_FLAG_CALLBACK_PRE | \
                        CO_CONFIG_FLAG_TIMERNEXT)
#endif

#ifndef CO_CONFIG_PDO
#define CO_CONFIG_PDO (CO_CONFIG_RPDO_ENABLE | \
                       CO_CONFIG_TPDO_ENABLE | \
                       CO_CONFIG_PDO_SYNC_ENABLE | \
                       CO_CONFIG_RPDO_CALLS_EXTENSION | \
                       CO_CONFIG_TPDO_CALLS_EXTENSION | \
                       CO_CONFIG_FLAG_CALLBACK_PRE | \
                       CO_CONFIG_FLAG_TIMERNEXT)
#endif

#ifndef CO_CONFIG_LEDS
#define CO_CONFIG_LEDS (CO_CONFIG_LEDS_ENABLE | \
                        CO_CONFIG_FLAG_TIMERNEXT)
#endif

#ifndef CO_CONFIG_GFC
#define CO_CONFIG_GFC (CO_CONFIG_GFC_ENABLE | \
                       CO_CONFIG_GFC_CONSUMER | \
                       CO_CONFIG_GFC_PRODUCER)
#endif

#ifndef CO_CONFIG_SRDO
#define CO_CONFIG_SRDO (CO_CONFIG_SRDO_ENABLE | \
                        CO_CONFIG_SRDO_CHECK_TX | \
                        CO_CONFIG_RSRDO_CALLS_EXTENSION | \
                        CO_CONFIG_TSRDO_CALLS_EXTENSION | \
                        CO_CONFIG_FLAG_CALLBACK_PRE | \
                        CO_CONFIG_FLAG_TIMERNEXT)
#endif

#ifndef CO_CONFIG_SRDO_MINIMUM_DELAY
#define CO_CONFIG_SRDO_MINIMUM_DELAY 0
#endif

#ifndef CO_CONFIG_LSS
#define CO_CONFIG_LSS (CO_CONFIG_LSS_SLAVE | \
                       CO_CONFIG_LSS_SLAVE_FASTSCAN_DIRECT_RESPOND | \
                       CO_CONFIG_LSS_MASTER | \
                       CO_CONFIG_FLAG_CALLBACK_PRE)
#endif

#ifndef CO_CONFIG_GTW
#define CO_CONFIG_GTW (CO_CONFIG_GTW_ASCII | \
                       CO_CONFIG_GTW_ASCII_SDO | \
                       CO_CONFIG_GTW_ASCII_NMT | \
                       CO_CONFIG_GTW_ASCII_LSS | \
                       CO_CONFIG_GTW_ASCII_LOG | \
                       CO_CONFIG_GTW_ASCII_ERROR_DESC | \
                       CO_CONFIG_GTW_ASCII_PRINT_HELP | \
                       CO_CONFIG_GTW_ASCII_PRINT_LEDS)
#define CO_CONFIG_GTW_BLOCK_DL_LOOP 1
#define CO_CONFIG_GTWA_COMM_BUF_SIZE 2000
#define CO_CONFIG_GTWA_LOG_BUF_SIZE 2000
#endif

#ifndef CO_CONFIG_CRC16
#define CO_CONFIG_CRC16 (CO_CONFIG_CRC16_ENABLE)
#endif

#ifndef CO_CONFIG_FIFO
#define CO_CONFIG_FIFO (CO_CONFIG_FIFO_ENABLE | \
                        CO_CONFIG_FIFO_ALT_READ | \
                        CO_CONFIG_FIFO_CRC16_CCITT | \
                        CO_CONFIG_FIFO_ASCII_COMMANDS | \
                        CO_CONFIG_FIFO_ASCII_DATATYPES)
#endif

#ifndef CO_CONFIG_TRACE
#define CO_CONFIG_TRACE (CO_CONFIG_TRACE_ENABLE)
#endif

/* skip this section for Doxygen, because it is documented in CO_driver.h */
#ifndef CO_DOXYGEN

#ifndef CONFIG_CO_USE_DYNAMIC_ALLOCATE
#define CO_USE_GLOBALS
#endif



#ifdef __LITTLE_ENDIAN
#define CO_LITTLE_ENDIAN
#define CO_SWAP_16(x) x
#define CO_SWAP_32(x) x
#define CO_SWAP_64(x) x

#else /* __BIG_ENDIAN */
#define CO_SWAP_16(x) __builtin_bswap16(x)
#define CO_SWAP_32(x) __builtin_bswap32(x)
#define CO_SWAP_64(x) __builtin_bswap64(x)
#endif

/* #define CO_USE_LEDS */
/* NULL is defined in stddef.h */
/* true and false are defined in stdbool.h */
/* int8_t to uint64_t are defined in stdint.h */
typedef unsigned char           bool_t;
typedef float                   float32_t;
typedef double                  float64_t;
typedef char                    char_t;
typedef unsigned char           oChar_t;
typedef unsigned char           domain_t;

/* CAN receive message structure as aligned in socketCAN. */
typedef struct {
    uint32_t ident;
    uint8_t DLC;
    uint8_t padding[3];
    uint8_t data[8];
} CO_CANrxMsg_t;

/* Access to received CAN message */
static inline uint16_t CO_CANrxMsg_readIdent(const CO_CANrxMsg_t *rxMsg)
{
    return (uint16_t)(rxMsg->ident & CAN_STD_ID_MASK);
}

static inline uint8_t CO_CANrxMsg_readDLC(const CO_CANrxMsg_t *rxMsg)
{
    return (uint8_t)rxMsg->DLC;
}

static inline uint8_t *CO_CANrxMsg_readData(const CO_CANrxMsg_t *rxMsg)
{
    return (uint8_t *)rxMsg->data;
}

/* Received message object */
typedef struct {
    uint32_t ident;
    uint32_t mask;
    int filter_id;
    void *object;
    void (*CANrx_callback)(void *object, void *msg);
} CO_CANrx_t;

/* Transmit message object as aligned in socketCAN. */
typedef struct {
    uint32_t ident;
    uint8_t DLC;
    uint8_t padding[3];     /* ensure alignment */
    uint8_t data[8];
    volatile bool_t bufferFull; /* not used */
    volatile bool_t syncFlag;   /* info about transmit message */
} CO_CANtx_t;

/* CAN device interface */
typedef struct {
    const char *devname;
    int fd;
} CO_CANdevice_t;

/* CAN module object */
typedef struct {
    CO_CANdevice_t *dev;
    CO_CANrx_t *rxArray;
    CO_CANtx_t *txArray;
    struct can_filter *rxFilter;
    void *em;
    uint16_t rxSize;
    uint16_t txSize;
    uint16_t CANerrorStatus;
    bool_t CANnormal;
    bool_t initialized;
} CO_CANmodule_t;


#ifdef CONFIG_CO_MULTITHREAD
extern rtems_mutex _co_can_send_lock;
extern rtems_mutex _co_can_emcy_lock;
extern rtems_mutex _co_lock;

/* (un)lock critical section in CO_CANsend() */
#define CO_LOCK_CAN_SEND()   rtems_mutex_lock(&_co_can_send_lock);
#define CO_UNLOCK_CAN_SEND() rtems_mutex_unlock(&_co_can_send_lock);

/* (un)lock critical section in CO_errorReport() or CO_errorReset() */
#define CO_LOCK_EMCY()       rtems_mutex_lock(&_co_can_emcy_lock);
#define CO_UNLOCK_EMCY()     rtems_mutex_unlock(&_co_can_emcy_lock);

/* (un)lock critical section when accessing Object Dictionary */
#define CO_LOCK_OD()         rtems_mutex_lock(&_co_lock);
#define CO_UNLOCK_OD()       rtems_mutex_unlock(&_co_lock);

#else
/* (un)lock critical section in CO_CANsend() */
#define CO_LOCK_CAN_SEND()
#define CO_UNLOCK_CAN_SEND()

/* (un)lock critical section in CO_errorReport() or CO_errorReset() */
#define CO_LOCK_EMCY()
#define CO_UNLOCK_EMCY()

/* (un)lock critical section when accessing Object Dictionary */
#define CO_LOCK_OD()
#define CO_UNLOCK_OD()
#endif /* CONFIG_CO_MULTITHREAD */

/* Synchronization between CAN receive and message processing threads. */
#define CO_MemoryBarrier()
#define CO_FLAG_READ(rxNew) ((rxNew) != NULL)
#define CO_FLAG_SET(rxNew) {CO_MemoryBarrier(); rxNew = (void*)1L;}
#define CO_FLAG_CLEAR(rxNew) {CO_MemoryBarrier(); rxNew = NULL;}
#endif /* CO_DOXYGEN */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CO_DRIVER_TARGET_H_ */
