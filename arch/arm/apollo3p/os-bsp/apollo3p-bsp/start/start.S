#include <rtems/asm.h>
#include <rtems/score/percpu.h>
#include <rtems/score/armv7m.h>

#include <bspopts.h>
#include <bsp/irq.h>


	/* Global symbols */
	.globl	_start
	.globl	bsp_start_hook_0_done
	.globl	bsp_start_vector_table_begin
	.globl	bsp_start_vector_table_end
	.globl	bsp_start_vector_table_size
	.globl	bsp_vector_table_size


	.section	".bsp_start_text", "ax"

#ifdef BSP_START_ZIMAGE_HEADER
.rept	8
	mov	r0, r0
.endr
	b	.Lafter_header
	.word	0x016f2818		/* Magic numbers to help the loader */
	.word	bsp_section_start_begin	/* zImage start address */
	.word	bsp_section_data_end	/* zImage end address */
.Lafter_header:
#endif /* BSP_START_ZIMAGE_HEADER */


	.syntax	unified
	.thumb

bsp_start_vector_table_begin:

	.word	_ISR_Stack_area_end
	.word	_start /* Reset */
	.word	_ARMV7M_Exception_default /* NMI */
	.word	_ARMV7M_Exception_default /* Hard Fault */
	.word	_ARMV7M_Exception_default /* MPU Fault */
	.word	_ARMV7M_Exception_default /* Bus Fault */
	.word	_ARMV7M_Exception_default /* Usage Fault */
	.word	_ARMV7M_Exception_default /* Reserved */
	.word	_ARMV7M_Exception_default /* Reserved */
	.word	_ARMV7M_Exception_default /* Reserved */
	.word	_ARMV7M_Exception_default /* Reserved */
	.word	_ARMV7M_Supervisor_call /* SVC */
	.word	_ARMV7M_Exception_default /* Debug Monitor */
	.word	_ARMV7M_Exception_default /* Reserved */
	.word	_ARMV7M_Pendable_service_call /* PendSV */
	.word	_ARMV7M_Clock_handler /* SysTick */
	.rept	BSP_INTERRUPT_VECTOR_MAX + 1
	.word	_ARMV7M_NVIC_Interrupt_dispatch /* IRQ */
	.endr
	.rept	47 - BSP_INTERRUPT_VECTOR_MAX
	.word	0 	/* Patch */
	.endr

bsp_start_vector_table_end:

	.thumb_func

_start:
	
	/* Set vector talbe base */
	ldr    r0, =0xE000ED08
	ldr    r1, =bsp_start_vector_table_begin
	str    r1, [r0]
		
#if defined(ARM_MULTILIB_VFP) && defined(ARM_MULTILIB_HAS_CPACR)
	/*
	 * Enable CP10 and CP11 coprocessors for privileged and user mode in
	 * CPACR (bits 20-23).  Ensure that write to register completes.
	 */
	ldr	r0, =ARMV7M_CPACR
	ldr	r1, [r0]
	orr	r1, r1, #(0xf << 20)
	str	r1, [r0]
	dsb
	isb
#endif
	ldr	sp, =_ISR_Stack_area_end
	
	bl  bsp_start_hook_1

	movs	r0, #0
	bl	boot_card
	
	bkpt

	.set	bsp_start_vector_table_size, bsp_start_vector_table_end - bsp_start_vector_table_begin
	.set	bsp_vector_table_size, bsp_start_vector_table_size
