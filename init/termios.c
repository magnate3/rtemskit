#include <rtems/libio.h>
#include <rtems/sysinit.h>

#ifdef CONFIG_TERMIOS_BUFSZ
static void termios_resize(void)
{
    rtems_termios_bufsize(CONFIG_TERMIOS_BUFSZ, 
                          CONFIG_TERMIOS_BUFSZ,
                          CONFIG_TERMIOS_BUFSZ);
}

RTEMS_SYSINIT_ITEM(termios_resize,
    RTEMS_SYSINIT_BSP_START, 
    RTEMS_SYSINIT_ORDER_MIDDLE);
#endif /* CONFIG_TERMIOS_BUFSZ */

