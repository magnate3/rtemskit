#include <rtems/shell.h>
#include <rtems/sysinit.h>

#include "base/reboot_listener.h"


static int shutdown_notify(struct observer_base *obs, 
    unsigned long action, void *data)
{   
    char *argv[] = {"/home/shutdown.joel"};
    rtems_shell_script_file(0, argv);
    return NOTIFY_DONE;
}

static struct observer_base shutdown_observer = {
    .update = shutdown_notify,
    .priority = 1,
};

static void machine_shutdown_init(void)
{
    reboot_notify_register(&shutdown_observer);
}

RTEMS_SYSINIT_ITEM(machine_shutdown_init,
    RTEMS_SYSINIT_DEVICE_DRIVERS,
    RTEMS_SYSINIT_ORDER_MIDDLE);

