AC_PREREQ([2.69])
AC_INIT([rtems-c-src-lib-libbsp-arm-stm32f7],[_RTEMS_VERSION],[https://devel.rtems.org/newticket])
RTEMS_TOP(../../../../../..)
RTEMS_SOURCE_TOP
RTEMS_BUILD_TOP
RTEMS_BSP_LINKCMDS

RTEMS_CANONICAL_TARGET_CPU
AM_INIT_AUTOMAKE([no-define nostdinc foreign 1.12.2])
RTEMS_BSP_CONFIGURE

RTEMS_BSPOPTS_SET([APOLLO_FAMILY_3PLUS],[apollo3p*],[1])
RTEMS_BSPOPTS_HELP([APOLLO_FAMILY_3PLUS],[Chip belongs to the APOLLO3P family.])

RTEMS_BSP_CLEANUP_OPTIONS

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
