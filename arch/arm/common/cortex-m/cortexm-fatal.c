
#include <rtems/bspIo.h>
#include <rtems/version.h>
#include <rtems/score/threadimpl.h>
#include <rtems/score/userextimpl.h>
#include <rtems/sysinit.h>
#include <rtems/fatal.h>
#include <inttypes.h>

#include "cm_backtrace.h"

static void cpu_fatal_extension(rtems_fatal_source source, bool unused,
    rtems_fatal_code code)
{
    const rtems_exception_frame *frame = (const rtems_exception_frame *)code;
    Thread_Control *executing;

    printk("\n*** FATAL ***\nfatal source: %i (%s)\n", source, 
    rtems_fatal_source_text( source ));

    if ( source == RTEMS_FATAL_SOURCE_EXCEPTION ) {
        rtems_exception_frame_print(frame);
    } else if ( source == INTERNAL_ERROR_CORE ) {
        printk("fatal code: %ju (%s)\n", (uintmax_t)code, 
            rtems_internal_error_text(code ));
    } else {
        printk("fatal code: %ju (0x%08jx)\n", (uintmax_t)code, (uintmax_t)code);
    }

    printk("RTEMS version: %s\nRTEMS tools: %s\n", rtems_version(), __VERSION__);
    executing = _Thread_Get_executing();
    if ( executing != NULL ) {
        char name[ 2 * THREAD_DEFAULT_MAXIMUM_NAME_SIZE ];
        _Thread_Get_name( executing, name, sizeof( name ) );
        printk("executing thread ID: 0x08%" PRIx32 "\n" "executing thread name: %s\n",
        executing->Object.id, name);
    } else {
        printk( "executing thread is NULL\n" );
    }


    /* Backtrace call stack */
    cm_backtrace_fault((uint32_t)frame->register_lr, frame->register_sp);

    /*
    *  Check both conditions -- if you want to ask for reboot, then
    *  you must have meant to reset the board.
    */
}


static void cpu_extension_init(void)
{
    static User_extensions_Control cpu_extension = {
        .Switch = NULL,
        .Callouts = {
            .fatal = cpu_fatal_extension
        }
    };
     
    cm_backtrace_init("RTEMS Application", "Cortex-Mx", "1.0.0");   
    _User_extensions_Add_set(&cpu_extension);
}

RTEMS_SYSINIT_ITEM(
    cpu_extension_init,
    RTEMS_SYSINIT_INITIAL_EXTENSIONS,
    RTEMS_SYSINIT_ORDER_LAST
);
