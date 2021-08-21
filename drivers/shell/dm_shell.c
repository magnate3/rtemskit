#include <string.h>
#include <errno.h>

#include <rtems/shell.h>
#include <rtems/sysinit.h>

#include "dm/util.h"

#define DM_HELP \
    "Driver model information\n" \
    "\n" \
    "dm [-a][--all]\n" \
    "   [-u][--uclass]\n" \
    "   [-d][--drivers]\n" \
    "   [-sd][--static-driver]\n" \
    "   [-dc][--driver-compat]\n"


static int shell_dm(int argc, char *argv[])
{
    if (argc == 1) {
        return -EINVAL;
    }
    if (argc == 2) {
        if (!strcmp(argv[1], "--all") ||
            !strcmp(argv[1], "-a")) {
            dm_dump_all();
        } else if (!strcmp(argv[1], "--uclass") ||
            !strcmp(argv[1], "-u")) {
            dm_dump_uclass();
        } else if (!strcmp(argv[1], "--drivers") ||
            !strcmp(argv[1], "-d")) {
            dm_dump_drivers();
        } else if (!strcmp(argv[1], "--static-driver") ||
            !strcmp(argv[1], "-sd")) {
            dm_dump_static_driver_info();
        } else if (!strcmp(argv[1], "--driver-compat") ||
            !strcmp(argv[1], "-dc")) {
            dm_dump_driver_compat();
        } else
            return -EINVAL;
    }        
    return 0;
}

static rtems_shell_cmd_t shell_dm_command = {
    "dm",         /* name */
    DM_HELP,      /* usage */
    "rtems",      /* topic */
    shell_dm,     /* command */
    NULL,         /* aliass */
    NULL          /* next */
};

static void shell_register(void)
{
    rtems_shell_add_cmd_struct(&shell_dm_command);
}

RTEMS_SYSINIT_ITEM(shell_register,
    RTEMS_SYSINIT_LAST,
    RTEMS_SYSINIT_ORDER_MIDDLE);
