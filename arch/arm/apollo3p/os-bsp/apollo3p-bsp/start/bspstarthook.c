#include <bsp.h>
#include <bsp/start.h>

void BSP_START_TEXT_SECTION bsp_start_hook_0(void)
{
  /* Do nothing */
}

void BSP_START_TEXT_SECTION bsp_start_hook_1(void)
{
  bsp_start_copy_sections();
  bsp_start_clear_bss();
}
