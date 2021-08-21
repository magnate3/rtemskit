/*
 * Copyright (c) 2012 Sebastian Huber.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Obere Lagerstr. 30
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#include <rtems/bspIo.h>
#include <rtems/console.h>


void *_BSP_Thread_Idle_body( uintptr_t ignored )
{
  while ( true ) {
    /* Do nothing */
  }

  return NULL;
}

void __attribute__((weak)) bsp_start( void )
{

}

rtems_status_code __attribute__((weak)) console_initialize(
    rtems_device_major_number major,
    rtems_device_minor_number minor,
    void *arg) {
        
}

static void __empty_putchar(char c)
{

}

BSP_output_char_function_type BSP_output_char = __empty_putchar;
BSP_polling_getchar_function_type BSP_poll_char = NULL;
