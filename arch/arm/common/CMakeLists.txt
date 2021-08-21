
add_subdirectory_ifdef(CONFIG_STM32_FAMILY  stm32cube)
add_subdirectory_ifdef(CONFIG_APOLLO_FAMILY ambiq-sdk)

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "cortex-m7"
  OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "cortex-m4"
  OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "cortex-m3")
  add_subdirectory(cm_backtrace)
endif()




