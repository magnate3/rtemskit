#Board stm32f7
rtems_library_named(board)

rtems_library_sources(
  stm32_board.c
  stm32_mpu.c
  stm32_pinmux.c
  ../common/cortex-m/cortexm-exception.c
)



