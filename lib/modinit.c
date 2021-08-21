#include "base/modinit.h"

#include <stdio.h>

RTEMS_LINKER_ROSET(module_app, struct module_operations);
RTEMS_LINKER_ROSET(module_driver, struct module_operations);

int _module_init(void)
{
    const struct module_operations *item;
    int ret = 0;
    
    RTEMS_LINKER_SET_FOREACH(module_app, item) {
        ret = item->init();
        if (ret == MOD_BAD) {
            printf("Warnning***: %s() -> %s() initialize failed.\n", 
                __func__, item->name);
        } else if (ret == MOD_STOP) {
            printf("Error***: %s() -> %s() initialize failed and stop\n", 
                __func__, item->name);
            break;
        }
    }
    return ret;
}

void _module_driver_init(void)
{
    const struct module_operations *item;
    
    RTEMS_LINKER_SET_FOREACH(module_driver, item) {
        item->init();
    }
}

