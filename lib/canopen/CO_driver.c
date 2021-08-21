#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include "301/CO_driver.h"
#include "301/CO_Emergency.h"


#ifdef CONFIG_CO_MULTITHREAD
rtems_mutex _co_can_send_lock = RTEMS_MUTEX_INITIALIZER("CANOPEN-SEND");
rtems_mutex _co_can_emcy_lock = RTEMS_MUTEX_INITIALIZER("CANOPEN-EMCY");
rtems_mutex _co_lock = RTEMS_MUTEX_INITIALIZER("CANOPEN");
#endif

static CO_ReturnError_t CO_CANdeviceOpen(CO_CANdevice_t *dev, 
    uint32_t bitrate)
{
    struct can_attr attr;
    int fd, ret;

    fd = open(dev->devname, O_RDWR);
    if (fd < 0) {
        printf("%s Device(%s) not exist\n", __func__, dev->devname);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    ioctl(fd, CAN_IOC_GET_ATTR, &attr);
    attr.mode = CAN_NORMAL_MODE;
    attr.bitrate = bitrate;
    ret = ioctl(fd, CAN_IOC_SET_ATTR, &attr);
    if (ret) {
        printf("%s Set CAN device(%s) failed\n", __func__, dev->devname);
        goto close_fd;
    }

    dev->fd = fd;
    return CO_ERROR_NO;
close_fd:
    close(fd);
    return CO_ERROR_INVALID_STATE;
}

static CO_ReturnError_t CO_CANdeviceClose(CO_CANdevice_t *dev)
{
    struct can_attr attr;
    int ret;

    attr.mode = CAN_SILENT_MODE;
    attr.bitrate = 0;
    ret = ioctl(dev->fd, CAN_IOC_SET_ATTR, &attr);
    if (ret) {
        printf("%s Set CAN device(%s) failed\n", __func__, dev->devname);
        return CO_ERROR_INVALID_STATE;
    }

    return CO_ERROR_NO;
}

void CO_CANsetConfigurationMode(void *CANptr)
{
    CO_CANdevice_t *dev = CANptr;

    ioctl(dev->fd, CAN_IOC_STOP, NULL);
}

void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule)
{
    CO_CANdevice_t *dev = CANmodule->dev;

    if (ioctl(dev->fd, CAN_IOC_START, NULL)) {
        printf("%s Can not start CAN(%s)\n", __func__, dev->devname);
        CANmodule->CANnormal = false;
    }

    CANmodule->CANnormal = true;
}

CO_ReturnError_t CO_CANmodule_init(
        CO_CANmodule_t        *CANmodule,
        void                  *CANptr,
        CO_CANrx_t            rxArray[],
        uint16_t              rxSize,
        CO_CANtx_t            txArray[],
        uint16_t              txSize,
        uint16_t              CANbitRate)
{
    CO_CANdevice_t *candev = CANptr;
    CO_ReturnError_t err;
    uint16_t i;

    if (CANmodule == NULL || rxArray == NULL || txArray == NULL) {
        printf("%s failed to initialize CAN module\n", __func__);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    if (CANmodule->initialized)
        return CO_ERROR_NO;

    if (candev->devname == NULL) {
        printf("%s invalid device name\n", __func__);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    if (rxSize > CONFIG_CAN_MAX_FILTER) {
        printf("insufficient number of concurrent CAN RX filters"
		" (needs %d, %d available)", rxSize, CONFIG_CAN_MAX_FILTER);
        return CO_ERROR_OUT_OF_MEMORY;
    }

    /* Configure object variables */
    CANmodule->dev = candev;
    CANmodule->rxArray = rxArray;
    CANmodule->rxSize = rxSize;
    CANmodule->txArray = txArray;
    CANmodule->txSize = txSize;
    CANmodule->CANnormal = false;
    CANmodule->CANerrorStatus = 0U;

    for (i = 0; i < rxSize; i++){
        rxArray[i].ident = 0U;
        rxArray[i].filter_id = CAN_NO_FREE_FILTER;
        rxArray[i].CANrx_callback = NULL;
        rxArray[i].object = NULL;
    }
    
    for (i = 0; i < txSize; i++)
        txArray[i].bufferFull = false;

    /* Configure CAN module registers */
    err = CO_CANdeviceOpen(candev, CANbitRate);
    if (err)
        return err;

    CANmodule->initialized = true;
    return CO_ERROR_NO;
}

void CO_CANmodule_disable(CO_CANmodule_t *CANmodule)
{
    if (CANmodule == NULL)
        return;
    
    if (!CANmodule->initialized)
        return;
        
    CO_CANrx_t *buffer = CANmodule->rxArray;
    for (int i = 0; i < CANmodule->rxSize; i++) {
        ioctl(buffer->filter_id, CAN_IOC_DETACH_FILTER, 
            &buffer->filter_id);
        buffer->filter_id = -1;
    }
    
    if (CO_CANdeviceClose(CANmodule->dev) == CO_ERROR_NO) {
        CANmodule->initialized = false;
        CANmodule->CANnormal = false;
    }
}

CO_ReturnError_t CO_CANrxBufferInit(
        CO_CANmodule_t         *CANmodule,
        uint16_t                index,
        uint16_t                ident,
        uint16_t                mask,
        bool_t                  rtr,
        void                   *object,
        void                  (*CANrx_callback)(void *object, void *message))
{
    struct can_attach filter;
    CO_CANdevice_t *dev;
    CO_CANrx_t *buffer;

    if (CANmodule == NULL || object == NULL)
        return CO_ERROR_ILLEGAL_ARGUMENT;

    if (!CANrx_callback || (index >= CANmodule->rxSize)) {
        printf("failed to initialize CAN rx buffer, illegal argument");
        CO_errorReport(CANmodule->em, CO_EM_GENERIC_SOFTWARE_ERROR,
            CO_EMC_SOFTWARE_INTERNAL, 0);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    dev = CANmodule->dev;

    /* buffer, which will be configured */
    buffer = &CANmodule->rxArray[index];

    /* Configure object variables */
    buffer->object = object;
    buffer->CANrx_callback = CANrx_callback;

    /* CAN identifier and CAN mask */
    buffer->ident = (ident & CAN_STD_ID_MASK) | (rtr? CAN_RTR_MASK: 0);
    buffer->mask =  (mask & CAN_STD_ID_MASK) | (rtr? CAN_RTR_MASK: 0);

    filter.cb = (can_filter_cb_t)buffer->CANrx_callback;
    filter.data = buffer->object;
    filter.filter.can_id = buffer->ident;
    filter.filter.can_mask = buffer->mask;
    filter.filter_id = -1;
    if (ioctl(dev->fd, CAN_IOC_ATTACH_FILTER, &filter)) {
        buffer->filter_id = -1;
        return CO_ERROR_SYSCALL;
    }

    buffer->filter_id = filter.filter_id;
    return CO_ERROR_NO;
}

CO_CANtx_t *CO_CANtxBufferInit(
        CO_CANmodule_t         *CANmodule,
        uint16_t                index,
        uint16_t                ident,
        bool_t                  rtr,
        uint8_t                 noOfBytes,
        bool_t                  syncFlag)
{
    CO_CANtx_t *buffer;

    if (CANmodule == NULL)
        return NULL;

    if (index >= CANmodule->txSize) {
        printf("failed to initialize CAN rx buffer, illegal argument");
        CO_errorReport(CANmodule->em, CO_EM_GENERIC_SOFTWARE_ERROR,
            CO_EMC_SOFTWARE_INTERNAL, 0);
        return NULL;
    }

    /* get specific buffer */
    buffer = &CANmodule->txArray[index];

    /* CAN identifier, DLC and rtr, bit aligned with CAN module transmit buffer.
    * Microcontroller specific. */
    buffer->ident = (ident & CAN_STD_ID_MASK) | (rtr? CAN_RTR_MASK: 0);
    buffer->DLC = noOfBytes;
    buffer->bufferFull = false;
    buffer->syncFlag = syncFlag;
    return buffer;
}

CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer)
{
    struct can_frame *msg = (struct can_frame *)buffer;
    CO_ReturnError_t err = CO_ERROR_NO;
    
	if (RTEMS_PREDICT_FALSE(!CANmodule || !CANmodule->dev || !buffer))
		return CO_ERROR_ILLEGAL_ARGUMENT;

    CO_LOCK_CAN_SEND();
    if (buffer->bufferFull) {
        CANmodule->CANerrorStatus = CO_CAN_ERRTX_OVERFLOW;
        buffer->bufferFull = false;
        err = CO_ERROR_TX_OVERFLOW;
        goto exit;
    }

    int ret = write(CANmodule->dev->fd, msg, sizeof(*msg));
    if (RTEMS_PREDICT_FALSE(ret == -EBUSY)) {
        buffer->bufferFull = true;
    } else if (RTEMS_PREDICT_FALSE(ret < 0)) {
		CO_errorReport(CANmodule->em, CO_EM_GENERIC_SOFTWARE_ERROR,
			       CO_EMC_COMMUNICATION, 0);
        err = CO_ERROR_TX_UNCONFIGURED;
    }

exit:
    CO_UNLOCK_CAN_SEND();
    return err;
}

void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule)
{
    bool_t tpdoDeleted = false;
    CO_CANtx_t *buffer;
    uint16_t i;

    if (RTEMS_PREDICT_FALSE(!CANmodule))
        return;

    CO_LOCK_CAN_SEND();
    for (i = 0; i < CANmodule->txSize; i++) {
        buffer = &CANmodule->txArray[i];
        if (buffer->bufferFull && buffer->syncFlag) {
            buffer->bufferFull = false;
            tpdoDeleted = true;
        }
    }

    CO_UNLOCK_CAN_SEND();
    if (tpdoDeleted) {
        CO_errorReport(CANmodule->em, CO_EM_TPDO_OUTSIDE_WINDOW,
            CO_EMC_COMMUNICATION, 0);
    }
}

