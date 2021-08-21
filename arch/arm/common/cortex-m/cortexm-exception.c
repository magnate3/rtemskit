#include <rtems/score/armv7m.h>
#include <rtems/bspIo.h>
#include <rtems/version.h>
#include <rtems/score/threadimpl.h>
#include <rtems/sysinit.h>
#include <rtems/fatal.h>
#include <inttypes.h>
#include <bsp/bootcard.h>

#include "base/compiler.h"
#include "cm_backtrace.h"


void _cortexm_fault(uint32_t stack_pointer, uint32_t link_addr)
{
    Thread_Control *executing;
    
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
    cm_backtrace_fault(link_addr, stack_pointer);
    bsp_reset();
}

static void __naked _cortexm_exception(void)
{
    __asm__ volatile(
        "mov r0, sp\n"
        "mov r1, lr\n"
        "b _cortexm_fault\n"
        :
        :
    );        
}

static void _cortexm_exception_init(void)
{
    _ARMV7M_Set_exception_priority_and_handler(ARMV7M_VECTOR_NMI,
        ARMV7M_EXCEPTION_PRIORITY_LOWEST, _cortexm_exception);
    _ARMV7M_Set_exception_priority_and_handler(ARMV7M_VECTOR_HARD_FAULT,
        ARMV7M_EXCEPTION_PRIORITY_LOWEST, _cortexm_exception);
    _ARMV7M_Set_exception_priority_and_handler(ARMV7M_VECTOR_MEM_MANAGE,
        ARMV7M_EXCEPTION_PRIORITY_LOWEST, _cortexm_exception);
    _ARMV7M_Set_exception_priority_and_handler(ARMV7M_VECTOR_HARD_FAULT,
        ARMV7M_EXCEPTION_PRIORITY_LOWEST, _cortexm_exception);
    _ARMV7M_Set_exception_priority_and_handler(ARMV7M_VECTOR_BUS_FAULT,
        ARMV7M_EXCEPTION_PRIORITY_LOWEST, _cortexm_exception);
    _ARMV7M_Set_exception_priority_and_handler(ARMV7M_VECTOR_USAGE_FAULT,
        ARMV7M_EXCEPTION_PRIORITY_LOWEST, _cortexm_exception);

    cm_backtrace_init("bin/rtems", "CortexM", "1.0.0");   
}

RTEMS_SYSINIT_ITEM(
    _cortexm_exception_init,
    RTEMS_SYSINIT_BSP_START,
    RTEMS_SYSINIT_ORDER_LAST
);


