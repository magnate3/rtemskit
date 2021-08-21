/*-
 * Copyright (c) 2010 Isilon Systems, Inc.
 * Copyright (c) 2010 iX Systems, Inc.
 * Copyright (c) 2010 Panasas, Inc.
 * Copyright (c) 2013, 2014 Mellanox Technologies, Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */
#ifndef	_LINUX_COMPLETION_H_
#define	_LINUX_COMPLETION_H_


#if !defined(__rtems_libbsd__)

#include <rtems/thread.h>

/*
 * struct completion - structure used to maintain state for a "completion"
 *
 * This is the opaque structure used to maintain the state for a "completion".
 * Completions currently use a FIFO to queue threads that have to wait for
 * the "completion" event.
 *
 * See also:  complete(), wait_for_completion() (and friends _timeout,
 * _interruptible, _interruptible_timeout, and _killable), init_completion(),
 * reinit_completion(), and macros DECLARE_COMPLETION(),
 * DECLARE_COMPLETION_ONSTACK().
 */
struct completion {
    rtems_id holder;
};

#define init_completion_map(x, m) __init_completion(x)
#define init_completion(x) __init_completion(x)
static inline void complete_acquire(struct completion *x) {}
static inline void complete_release(struct completion *x) {}

#define COMPLETION_INITIALIZER(...) 
#define COMPLETION_INITIALIZER_ONSTACK_MAP(...) 
#define COMPLETION_INITIALIZER_ONSTACK(...)


/**
 * DECLARE_COMPLETION - declare and initialize a completion structure
 * @work:  identifier for the completion structure
 *
 * This macro declares and initializes a completion structure. Generally used
 * for static declarations. You should use the _ONSTACK variant for automatic
 * variables.
 */
#define DECLARE_COMPLETION(work) \
	struct completion work = {0}

/*
 * Lockdep needs to run a non-constant initializer for on-stack
 * completions - so we use the _ONSTACK() variant for those that
 * are on the kernel stack:
 */
/**
 * DECLARE_COMPLETION_ONSTACK - declare and initialize a completion structure
 * @work:  identifier for the completion structure
 *
 * This macro declares and initializes a completion structure on the kernel
 * stack.
 */
#ifdef CONFIG_LOCKDEP
# define DECLARE_COMPLETION_ONSTACK(work) \
	struct completion work = COMPLETION_INITIALIZER_ONSTACK(work)
# define DECLARE_COMPLETION_ONSTACK_MAP(work, map) \
	struct completion work = COMPLETION_INITIALIZER_ONSTACK_MAP(work, map)
#else
# define DECLARE_COMPLETION_ONSTACK(work) DECLARE_COMPLETION(work)
# define DECLARE_COMPLETION_ONSTACK_MAP(work, map) DECLARE_COMPLETION(work)
#endif

/**
 * init_completion - Initialize a dynamically allocated completion
 * @x:  pointer to completion structure that is to be initialized
 *
 * This inline function will initialize a dynamically created completion
 * structure.
 */
static inline void __init_completion(struct completion *x)
{
    x->holder = rtems_task_self();
}

/**
 * reinit_completion - reinitialize a completion structure
 * @x:  pointer to completion structure that is to be reinitialized
 *
 * This inline function should be used to reinitialize a completion structure so it can
 * be reused. This is especially important after complete_all() is used.
 */
static inline void reinit_completion(struct completion *x)
{
	__init_completion(x);
}


static inline void 
complete(struct completion *x)
{
    rtems_event_transient_send(x->holder);
}

static inline void 
wait_for_completion(struct completion *x)
{
    (void) x;
    rtems_event_transient_receive(RTEMS_WAIT, RTEMS_NO_TIMEOUT);
}

static inline bool 
try_wait_for_completion(struct completion *x)
{
    rtems_status_code sc;

    (void) x;
    sc = rtems_event_transient_receive(RTEMS_NO_WAIT, 0);
    return sc == RTEMS_SUCCESSFUL;
}

static inline unsigned long 
wait_for_completion_timeout(struct completion *x, unsigned long timeout)
{
    rtems_status_code sc;
    unsigned long start;
    unsigned long end;

    (void) x;
    start = rtems_clock_get_ticks_since_boot();
    sc = rtems_event_transient_receive(RTEMS_WAIT, (rtems_interval)timeout);
    if (sc == RTEMS_TIMEOUT)
        return 0;
    end = rtems_clock_get_ticks_since_boot();
    return timeout - (unsigned long)((long)end - (long)start);
}
    

#define wait_for_completion_io(x) \
    wait_for_completion(x)
#define wait_for_completion_interruptible(x) \
    ({wait_for_completion(x); 0;})
#define wait_for_completion_killable(x) \
    wait_for_completion_interruptible(x)
#define wait_for_completion_io_timeout(x, timeout) \
    wait_for_completion_timeout(x, timeout)
#define wait_for_completion_interruptible_timeout(x, timeout) \
    (long)wait_for_completion_timeout(x, timeout)
#define wait_for_completion_killable_timeout(x, timeout) \
    (long)wait_for_completion_timeout(x, timeout)
    
#else /* __rtems_libbsd__ */
#include <linux/errno.h>

struct completion {
	unsigned int done;
};

#define	INIT_COMPLETION(c) \
	((c).done = 0)
#define	init_completion(c) \
	((c)->done = 0)
#define reinit_completion(c) \
    init_completion(c)
#define	complete(c)				\
	linux_complete_common((c), 0)
#define	complete_all(c)				\
	linux_complete_common((c), 1)
#define	wait_for_completion(c)			\
	linux_wait_for_common((c), 0)
#define	wait_for_completion_interuptible(c)	\
	linux_wait_for_common((c), 1)
#define	wait_for_completion_timeout(c, timeout)	\
	linux_wait_for_timeout_common((c), (timeout), 0)
#define	wait_for_completion_interruptible_timeout(c, timeout)	\
	linux_wait_for_timeout_common((c), (timeout), 1)
#define	try_wait_for_completion(c) \
	linux_try_wait_for_completion(c)
#define	completion_done(c) \
	linux_completion_done(c)

extern void linux_complete_common(struct completion *, int);
extern long linux_wait_for_common(struct completion *, int);
extern long linux_wait_for_timeout_common(struct completion *, long, int);
extern int linux_try_wait_for_completion(struct completion *);
extern int linux_completion_done(struct completion *);
#endif /* !__rtems_libbsd__ */
#endif					/* _LINUX_COMPLETION_H_ */
