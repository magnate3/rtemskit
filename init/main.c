#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <rtems.h>
#include <rtems/shell.h>
#include <rtems/media.h>
#include <rtems/fatal.h>
#include <rtems/confdefs.h>

#ifdef CONFIGURE_BSP_STACK_ALLOCATOR
#include <bsp/stackalloc.h>
#endif

#if defined (__rtems_libbsd__)
#include <rtems/bsd/bsd.h>
#include <machine/rtems-bsd-config.h>
#endif

#include "base/modinit.h"

#define ROOTFS_DIR "root"
#define MEDIA_MOUNTED_EVENT RTEMS_EVENT_10


#if defined(CONFIGURE_MEDIA_SERVICE)
struct dirlink {
    const char *target;
    const char *linkname;
};

static const struct dirlink link_table[] = {
    {ROOTFS_DIR"/etc", "/etc"},
    {ROOTFS_DIR"/lib", "/lib"},
};

static rtems_status_code  media_listener(rtems_media_event event, 
    rtems_media_state state, const char *src, const char *dest, void *arg) {
    if (event == RTEMS_MEDIA_EVENT_MOUNT &&
        state == RTEMS_MEDIA_STATE_SUCCESS) {
        rtems_id waiter = *(rtems_id *)arg;
        char *rm_argv[] = {"rm", "-rf", "/etc"};
        char linkname[128];
        int ret;
        
        ret = rtems_shell_main_rm(RTEMS_ARRAY_SIZE(rm_argv), rm_argv);
        if (ret) {
            printf("***Warning: remove /etc failed: %d\n", ret);
            return RTEMS_INCORRECT_STATE;
        }

        for (int i = 0; i < RTEMS_ARRAY_SIZE(link_table); i++) {
            snprintf(linkname, sizeof(linkname), "%s/%s", 
                dest, link_table[i].target);
            printf("symlink: %s -> %s\n", link_table[i].linkname, linkname);
            ret = symlink(linkname, link_table[i].linkname);
            if (ret) {
                printf("***Waring: create link failed(%s -> %s) with %d\n", 
                    link_table[i].target, link_table[i].linkname, ret);
            }
        }
        rtems_event_send(waiter, MEDIA_MOUNTED_EVENT);
    }
    return RTEMS_SUCCESSFUL;
}

static int media_service_init(void) {
    rtems_status_code sc;
    sc = rtems_media_initialize();
    if (sc) {
        printf("***Error: Media initialize failed: %s\n",
            rtems_status_text(sc));
        return rtems_status_code_to_errno(sc);
    }
    sc = rtems_media_server_initialize(110, 8192,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES);
    if (sc) {
        printf("***Error: Media service start failed: %s\n", 
            rtems_status_text(sc));
        return rtems_status_code_to_errno(sc);
    }
    return 0;
}
#endif

/* 
 * RTEMS application entry.
 * @arg: user argument
 */
rtems_task Init(rtems_task_argument arg) {
#if defined(CONFIGURE_MEDIA_SERVICE)
    rtems_id thread = rtems_task_self();
#endif
    rtems_status_code sc;
    _module_driver_init();
#if defined(CONFIGURE_MEDIA_SERVICE)
    if (media_service_init()) 
        rtems_panic("Media initialize failed\n");
    rtems_media_listener_add(media_listener, &thread);
#endif
#if defined (__rtems_libbsd__)
   /* 
    * Initialize libbsd subsystem. It's contains network, usb, mmc, 
    * netcmd and so on. 
    */
    rtems_task_priority old_prio;
	rtems_bsd_setlogpriority(CONFIG_LOG_LEVEL);
    rtems_task_set_priority(RTEMS_SELF, 110, &old_prio);
    (void)old_prio;
    sc = rtems_bsd_initialize();
    if (sc == RTEMS_SUCCESSFUL) {
#if defined(CONFIGURE_MEDIA_SERVICE)
        rtems_event_set evt;
        rtems_event_receive(MEDIA_MOUNTED_EVENT, 
            RTEMS_EVENT_ALL|RTEMS_WAIT, 
            RTEMS_MILLISECONDS_TO_TICKS(3000),
            &evt);
        (void) evt;  
#else
        rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(1000));
#endif
        /* Execute /etc/rc.conf script */
        rtems_bsd_run_etc_rc_conf(RTEMS_MILLISECONDS_TO_TICKS(10000), true);
    }

#elif defined(CONFIG_NET)
    extern int _net_init(void);
    /* Initialize rtems local network services*/
    int ret = _net_init();
    if (ret)
        printf("%s: Initialize network failed\n", __func__);
#else
    (void) sc;
#endif
    _module_init();
    rtems_task_exit();
    /* Never reached here !!*/
}

#if defined (__rtems_libbsd__) && defined(CONFIGURE_FDT)
RTEMS_BSD_DEFINE_NEXUS_DEVICE(ofwbus, 0, 0, NULL);
SYSINIT_DRIVER_REFERENCE(simplebus, ofwbus);
#endif
