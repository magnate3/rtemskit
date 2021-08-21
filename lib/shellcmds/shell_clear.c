#include <stdio.h>
#include <errno.h>

#include <rtems/shell.h>
#include <rtems/sysinit.h>

#include "shell_vt100.h"


/* Macro to send VT100 commands. */
#define SHELL_VT100_CMD(_cmd_) \
    do {				   \
        static const char cmd[] = _cmd_; \
        fprintf(stdout, "%s", cmd);	\
    } while (0)


static int shell_main_clear(int argc, char *argv[])
{
    if (argc > 1)
        return -EINVAL;

    SHELL_VT100_CMD(SHELL_VT100_CURSORHOME);
    SHELL_VT100_CMD(SHELL_VT100_CLEARSCREEN);
    return 0;
}

static void shell_clear_register(void)
{
    static rtems_shell_cmd_t shell_clear_command = {
        "clear",                         /* name */
        "clear     # Clear screen",      /* usage */
        "rtems",                         /* topic */
        shell_main_clear,                /* command */
        NULL,                            /* aliass */
        NULL                             /* next */
    };

    rtems_shell_add_cmd_struct(&shell_clear_command);
}

RTEMS_SYSINIT_ITEM(shell_clear_register,
    RTEMS_SYSINIT_LAST,
    RTEMS_SYSINIT_ORDER_MIDDLE);

