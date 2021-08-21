#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include <rtems.h>
#include <rtems/shell.h>
#include <rtems/imfs.h>
#include <rtems/console.h>

#include "base/modinit.h"

#define CONFIGURE_SHELL_COMMANDS_INIT
#include <rtems/shellconfig.h>

#ifndef CONFIG_SHELL_DEVICE
#define CONFIG_SHELL_DEVICE CONSOLE_DEVICE_NAME
#endif

#define START_SCRIPT "/etc/init.joel"
#define SCRIPT_SIZE (sizeof(CONFIG_SHELL_SCRIPT_TEXT) - 1)

#if defined(CONFIG_LIBDL) 
#if defined(CONFIGURE_SHELL_COMMAND_RAP)
#include <rtems/rtl/rap-shell.h>
static struct rtems_shell_cmd_tt rtems_rap_command = {
  .name     = "rap",
  .usage   = "Runtime load",
  .topic   = "rtems",
  .command = shell_rap,
  .alias   = NULL,
  .next    = NULL
};
#endif /* CONFIGURE_SHELL_COMMAND_RAP */

#if defined(CONFIGURE_SHELL_COMMAND_RTL)
#include <rtems/rtl/rtl-shell.h>
static struct rtems_shell_cmd_tt rtems_rtl_command = {
  .name     = "rtl",
  .usage   = "Runtime Linker",
  .topic   = "rtems",
  .command = rtems_rtl_shell_command,
  .alias   = NULL,
  .next    = NULL
};
#endif /* CONFIGURE_SHELL_COMMAND_RTL */
#endif /* CONFIG_LIBDL */

#if defined(CONFIGURE_SHELL_COMMAND_REBOOT)
static int shell_main_reboot(int argc, char *argv[])
{
    extern void bsp_reset(void);
    if (argc > 1)
        return -EINVAL;
    bsp_reset();
    return 0;
}

static rtems_shell_cmd_t shell_reboot_command = {
    "reboot",                                   /* name */
    "Reboot system immidateliy",                /* usage */
    "rtems",                                    /* topic */
    shell_main_reboot,                          /* command */
    NULL,                                       /* alias */
    NULL                                        /* next */
};
#endif /* CONFIG_SHELL_REBOOT */

#if defined(__rtems_libbsd__)
static void bsd_shell_init(void)
{
#if defined(CONFIGURE_BSD_SHELL_COMMAND_HOSTNAME)
    extern rtems_shell_cmd_t rtems_shell_HOSTNAME_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_HOSTNAME_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_ARP)
    extern rtems_shell_cmd_t rtems_shell_ARP_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_ARP_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_DHCPCD)
    extern rtems_shell_cmd_t rtems_shell_DHCPCD_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_DHCPCD_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_I2C)
    extern rtems_shell_cmd_t rtems_shell_I2C_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_I2C_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_IFCONFIG)
    extern rtems_shell_cmd_t rtems_shell_IFCONFIG_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_IFCONFIG_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_NETSTAT)
    extern rtems_shell_cmd_t rtems_shell_NETSTAT_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_NETSTAT_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_NVMECONTROL)
    extern rtems_shell_cmd_t rtems_shell_NVMECONTROL_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_NVMECONTROL_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_OPENSSL)
    extern rtems_shell_cmd_t rtems_shell_OPENSSL_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_OPENSSL_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_PFCTL)
    extern rtems_shell_cmd_t rtems_shell_PFCTL_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_PFCTL_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_PING)
    extern rtems_shell_cmd_t rtems_shell_PING_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_PING_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_RACOON)
    extern rtems_shell_cmd_t rtems_shell_RACOON_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_RACOON_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_ROUTE)
    extern rtems_shell_cmd_t rtems_shell_ROUTE_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_ROUTE_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_SETKEY)
    extern rtems_shell_cmd_t rtems_shell_SETKEY_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_SETKEY_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_STTY)
    extern rtems_shell_cmd_t rtems_shell_STTY_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_STTY_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_SYSCTL)
    extern rtems_shell_cmd_t rtems_shell_SYSCTL_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_SYSCTL_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_TCPDUMP)
    extern rtems_shell_cmd_t rtems_shell_TCPDUMP_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_TCPDUMP_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_VMSTAT)
    extern rtems_shell_cmd_t rtems_shell_VMSTAT_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_VMSTAT_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_WLANSTATS)
    extern rtems_shell_cmd_t rtems_shell_WLANSTATS_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_WLANSTATS_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_WPA_SUPPLICANT)
    extern rtems_shell_cmd_t rtems_shell_WPA_SUPPLICANT_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_WPA_SUPPLICANT_Command);
#endif
#if defined(CONFIGURE_BSD_SHELL_COMMAND_WPA_SUPPLICANT_FORK)
    extern rtems_shell_cmd_t rtems_shell_WPA_SUPPLICANT_FORK_Command;
    rtems_shell_add_cmd_struct(&rtems_shell_WPA_SUPPLICANT_FORK_Command);
#endif
}
#endif /* __rtems_libbsd__ */

static void misc_shell_init(void)
{
#if defined(CONFIGURE_SHELL_COMMAND_IRQ)
    extern rtems_shell_cmd_t bsp_interrupt_shell_command;
    rtems_shell_add_cmd_struct(&bsp_interrupt_shell_command);        
#endif 
#if defined(CONFIG_LIBDL) 
#if defined(CONFIGURE_SHELL_COMMAND_RAP)
    rtems_shell_add_cmd_struct(&rtems_rap_command);
#endif
#if defined(CONFIGURE_SHELL_COMMAND_RTL)
    rtems_shell_add_cmd_struct(&rtems_rtl_command);
#endif
#endif /* CONFIG_LIBDL */

#if defined(CONFIGURE_SHELL_COMMAND_REBOOT)
    rtems_shell_add_cmd_struct(&shell_reboot_command);
#endif
}

#if defined(CONFIG_SHELL_SCRIPT_TEXT)
static int create_file(const char *path, mode_t mode,
    const char *content, size_t size)
{
    return IMFS_make_linearfile(path, mode, content, size);
}

static int run_starup_script(void)
{
    char *script_file[] = { START_SCRIPT };
    int ret;

#ifdef CONFIG_LIBDL
    ret = create_file("/etc/libdl.conf", 0666, 
        CONFIG_LIBDL_CONTENT, sizeof(CONFIG_LIBDL_CONTENT)-1);
    if (ret) {
        printf("Error: Create libdl archive configurate file failed\n");
    }
#endif
    ret = create_file(START_SCRIPT, 0777, 
        CONFIG_SHELL_SCRIPT_TEXT, SCRIPT_SIZE);
    if (!ret) {
        ret = rtems_shell_script_file(0, script_file);
        if (ret) {
            printf("Execute initialize script(%s) failed: %d\n", 
                START_SCRIPT, ret);
        }
    }
    return ret;
}
#endif /* CONFIG_SHELL_SCRIPT_TEXT */

static int shell_init(void)
{
    rtems_task_priority prio;
    rtems_status_code sc;
#if defined(__rtems_libbsd__)
    bsd_shell_init();
#endif
#if defined(CONFIG_SHELL_PRIORITY)
    prio = CONFIG_SHELL_PRIORITY;
#else
    prio = RTEMS_MAXIMUM_PRIORITY - 1;
#endif /* CONFIG_SHELL_PRIORITY */

    misc_shell_init();
    sc = rtems_shell_init("SHLL", CONFIG_SHELL_STACK_SIZE, 
        prio, CONFIG_SHELL_DEVICE, false, false, NULL);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("Shell initialize failed(%s)\n", rtems_status_text(sc));
        return rtems_status_code_to_errno(sc);
    }
#if defined(CONFIG_SHELL_SCRIPT_TEXT)
    return run_starup_script();
#else
    return 0;
#endif
}

module_init(shell_init, 
            MOD_BASE, FIRST_ORDER);
