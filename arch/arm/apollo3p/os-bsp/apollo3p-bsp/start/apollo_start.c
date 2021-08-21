#include <rtems/score/percpu.h>
#include <rtems/score/armv7m.h>
#include <rtems/score/basedefs.h>

#include <bsp.h>
#include <bsp/start.h>
#include <bsp/bootcard.h>
#include <bsp/irq.h>

#include <bsp/armv7m-irq.h>



struct vector_tbl {
	unsigned int *initial_sp_value; /**< Initial stack pointer value. */
	ARMV7M_Exception_handler reset;
	ARMV7M_Exception_handler nmi;
	ARMV7M_Exception_handler hard_fault;
	ARMV7M_Exception_handler memory_manage_fault;
	ARMV7M_Exception_handler bus_fault;
	ARMV7M_Exception_handler usage_fault;
	ARMV7M_Exception_handler reserved_x001c[4];
	ARMV7M_Exception_handler sv_call;
	ARMV7M_Exception_handler debug_monitor;
	ARMV7M_Exception_handler reserved_x0034;
	ARMV7M_Exception_handler pend_sv;
	ARMV7M_Exception_handler systick;
	ARMV7M_Exception_handler irq[BSP_INTERRUPT_VECTOR_MAX + 1];
	ARMV7M_Exception_handler patch[47 - BSP_INTERRUPT_VECTOR_MAX];
};


unsigned int _bsp_vector_start;
unsigned int _bsp_vector_size;


void _start(void);


RTEMS_SECTION(".bsp_start_text") RTEMS_USED
const struct vector_tbl k_vector_table = {
	.initial_sp_value		= (unsigned int *)_ISR_Stack_area_end,
	.reset					= _start,
	.nmi					= _ARMV7M_Exception_default,
	.hard_fault				= _ARMV7M_Exception_default,
	.memory_manage_fault	= _ARMV7M_Exception_default,
	.bus_fault				= _ARMV7M_Exception_default,
	.usage_fault			= _ARMV7M_Exception_default,
	.sv_call				= _ARMV7M_Supervisor_call,
	.debug_monitor			= _ARMV7M_Exception_default,
	.pend_sv				= _ARMV7M_Pendable_service_call,
	.systick				= _ARMV7M_Clock_handler,
	.irq[0 ... BSP_INTERRUPT_VECTOR_MAX] = _ARMV7M_NVIC_Interrupt_dispatch,
	.patch[0 ... 47 - BSP_INTERRUPT_VECTOR_MAX]	= NULL
};

void RTEMS_SECTION(".bsp_start_text.1") _start(void)
{
	/* Set vector talbe base */
	__asm volatile (
		"ldr    r0, =0xE000ED08\n"
		"ldr    r1, =k_vector_table\n"
		"str    r1, [r0]"
	);

#if defined(ARM_MULTILIB_VFP) && defined(ARM_MULTILIB_HAS_CPACR)
	/* Enable FPU */
	__asm volatile (
		"ldr  r0, =0xE000ED88\n"
		"ldr  r1,[r0]\n"
		"orr  r1,#(0xF << 20)\n"
		"str  r1,[r0]\n"
		"dsb\n"
		"isb\n"
	);	
#endif

	/* Initialize data and bss section */
	bsp_start_clear_bss();
	bsp_start_copy_sections();

	_bsp_vector_start = (unsigned int)&k_vector_table;
	_bsp_vector_size  = sizeof(k_vector_table) - sizeof(k_vector_table.patch);
	
	boot_card(NULL);

	__asm volatile ("bkpt");
}



