#ifndef LINUX_LOCK_CLASS_H_
#define LINUX_LOCK_CLASS_H_

#include <rtems/thread.h>

#ifdef __cplusplus
extern "C"{
#endif


struct mutex {
    rtems_mutex lock;
};


static inline void mutex_init(struct mutex *mutex)
{
    rtems_mutex_init(&mutex->lock, "BASE-MUTEX");
}

static inline void mutex_lock(struct mutex *mutex)
{
    rtems_mutex_lock(&mutex->lock);
}

static inline void mutex_unlock(struct mutex *mutex)
{
    rtems_mutex_unlock(&mutex->lock);
}


struct semaphore {
    rtems_counting_semaphore sem;
};

#define down_interruptible(_sem) ({down(_sem); 0;})

static inline void sema_init(struct semaphore *sem, int val)
{
    rtems_counting_semaphore_init(&sem->sem, "BASE-SEM", 
        (unsigned int)val);
}

static inline void down(struct semaphore *sem)
{
    rtems_counting_semaphore_wait(&sem->sem);
}

static inline void up(struct semaphore *sem)
{
    rtems_counting_semaphore_post(&sem->sem);
}
    
    

#ifdef __cplusplus
}
#endif
#endif /* LINUX_LOCK_CLASS_H_ */

