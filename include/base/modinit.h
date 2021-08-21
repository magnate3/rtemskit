#ifndef RTEMS_APP_INIT_H_
#define RTEMS_APP_INIT_H_

#include <rtems/linkersets.h>
#include <rtems/sysinit.h>

#ifdef __cplusplus
extern "C"{
#endif

/*
 * 
 * Module class
 */
#define MOD_BASE          0010
#define MOD_SERVICE       0040
#define MOD_COMPONENT     0080
#define MOD_APPLICATION   00b0

/*
 * Initialization order
 */
#define FIRST_ORDER       0001
#define MIDDLE_ORDER      0080
#define LAST_ORDER        00ff


/*
 * Module initialize return code
 */
#define MOD_OK   0x0
#define MOD_BAD  0x1
#define MOD_STOP 0x80

struct module_operations {
    int (*init)(void);
    const char *name;
};

#define _RTEMS_MODULE_INDEX_ITEM(name, handler, _section, index) \
    enum { _section##_##handler = index }; \
    RTEMS_LINKER_ROSET_ITEM_ORDERED( \
    _section, \
    struct module_operations, \
    handler, \
    index) = { handler, name }
    
#define _RTEMS_MODULE_ITEM(name, handler, _section, _module, _order) \
  _RTEMS_MODULE_INDEX_ITEM(name, handler, _section, 0x##_module##_order)

#define module_init(handler, _module, _order) \
  _RTEMS_MODULE_ITEM(#handler, handler, module_app, _module, _order)
  
#define module_driver(handler, _module, _order) \
  _RTEMS_MODULE_ITEM(#handler, handler, module_driver, _module, _order)


int _module_init(void);
void _module_driver_init(void);

#ifdef __cplusplus
}
#endif
#endif /* RTEMS_APP_INIT_H_ */
