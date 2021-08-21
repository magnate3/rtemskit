
rtems_library_named(board_hal)

rtems_library_sources(
    stm32cube_hal.c
)

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "cortex-m7")
  rtems_compile_definitions(
      -DCORE_CM7
      -DSTM32F746xx
  )
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "cortex-m4")
  rtems_compile_definitions(
      -DCORE_CM4
  )
endif()

rtems_compile_definitions(
    -DUSE_HAL_DRIVER
    -DUSE_FULL_LL_DRIVER
)

set (stm32_socs
    stm32f1
    stm32f2
    stm32f3
    stm32f4
    stm32f7
    stm32h7
)

string(REGEX MATCH "^[A-Za-z0-9]+" board ${RTEMS_BOARD})
foreach(stm_soc ${stm32_socs})
   if (${board} STREQUAL ${stm_soc})
	rtems_include_directories(
	    ${stm_soc}xx/soc
	    ${stm_soc}xx/drivers/include
	    ${stm_soc}xx/drivers/include/Legacy
	)
   
       add_subdirectory(${stm_soc}xx)   
   endif()
endforeach()

rtems_include_directories(common_ll/include)


