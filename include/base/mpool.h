#ifndef BASE_MEM_POOL_H_
#define BASE_MEM_POOL_H_

#include <rtems/rtems/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"{
#endif

struct mpool_struct {
    rtems_id id;
    void *start;
    bool allocated;
};

int  mpool_create(struct mpool_struct *mp, void *start, size_t size);
void mpool_destroy(struct mpool_struct *mp);
void *mpool_alloc(struct mpool_struct *mp, size_t size, 
    unsigned long timeout);
void mpool_free(struct mpool_struct *mp, void *ptr);


int mblock_create(struct mpool_struct *mp, void *start, 
    int nblks, size_t blksize);
void *mblock_alloc(struct mpool_struct *mp);
void mblock_free(struct mpool_struct *mp, void *ptr);
void mblock_destroy(struct mpool_struct *mp);

#ifdef __cplusplus
}
#endif
#endif /* BASE_MEM_POOL_H_ */
