#ifndef DM_SERIAL_H_
#define DM_SERIAL_H_

#include <rtems/termiostypes.h>
#include <sys/ttycom.h>

#ifdef __cplusplus
extern "C"{
#endif

/**
 * struct struct dm_serial_ops - Driver model serial operations
 *
 * The uclass interface is implemented by all serial devices which use
 * driver model.
 */
struct dm_serial_ops {
	int  (*open)(struct udevice *dev);
	int  (*release)(struct udevice *dev);
	int  (*tx_empty)(struct udevice *dev);
	ssize_t (*fill_fifo)(struct udevice *dev, const char *buf, size_t size);
	void (*tx_irqena)(struct udevice *dev);
	void (*tx_irqdis)(struct udevice *dev);
	int  (*set_termios)(struct udevice *dev, const struct termios *tm);
	void (*set_mctrl)(struct udevice *dev, unsigned int mctrl);
	unsigned int (*get_mctrl)(struct udevice *dev);
	int  (*putc_poll)(struct udevice *dev, const char ch);
	int  (*getc_poll)(struct udevice *dev);
};

/* Access the serial operations for a device */
#define serial_get_ops(dev)	((struct dm_serial_ops *)(dev)->driver->ops)
	
/*
 * Called by serial interrupt service
 */
int serial_class_enqueue(struct udevice *dev, const char *buf, int len);
int serial_class_dequeue(struct udevice *dev);

#ifdef __cplusplus
}
#endif
#endif /* DM_SERIAL_H_ */
