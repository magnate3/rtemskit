#ifndef DM_OF_IRQ_H_
#define DM_OF_IRQ_H_

#include <linker_lists.h>

#ifdef __cplusplus
extern "C"{
#endif

struct udevice_id;
struct of_phandle_args;

struct of_irq_ops {
    const struct udevice_id *match_ids;
    int (*translate)(const struct of_phandle_args *ofirq,
            unsigned int *hwirq, unsigned int *type);
};

#define OF_IRQ(__name)	\
	static __ll_entry_declare(const struct of_irq_ops, \
	__name, of_irq_ops, ro)

#define OF_IRQ_START() \
	__ll_start(const struct of_irq_ops, ro)

#define OF_IRQ_END()  \
	__ll_end(const struct of_irq_ops, ro)



#ifdef __cplusplus
}
#endif
#endif /* DM_OF_IRQ_H_ */
