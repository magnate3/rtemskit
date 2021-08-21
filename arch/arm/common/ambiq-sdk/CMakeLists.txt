
rtems_library_named(board_hal)

rtems_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_ble.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_ble_patch_b0.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_burst.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_cachectrl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_clkgen.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_cmdq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_ctimer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_debug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_global.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_interrupt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_iom.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_ios.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_itm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_mcuctrl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_mspi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_pdm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_pwrctrl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_reset.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_scard.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_secure_ota.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_security.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_stimer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_sysctrl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_systick.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_tpiu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/${RTEMS_BOARD}/hal/am_hal_wdt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/am_util_delay.c
)

rtems_include_directories(
    CMSIS/AmbiqMicro/Include
    CMSIS/ARM/Include
    mcu/${RTEMS_BOARD}
    mcu/${RTEMS_BOARD}/hal
    mcu/${RTEMS_BOARD}/reg
    utils
)

rtems_library_compile_definitions(board
   -DAM_HAL_DISABLE_API_VALIDATION
)
rtems_library_compile_options(-Og)

