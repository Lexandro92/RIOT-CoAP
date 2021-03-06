/**
 * Copyright (C) 2015 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @ingroup xtimer
 * @{
 * @file
 * @brief xtimer convenience functionality
 * @author Kaspar Schleiser <kaspar@schleiser.de>
 * @}
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "xtimer.h"
#include "mutex.h"
#include "thread.h"
#include "irq.h"

#include "timex.h"

#define ENABLE_DEBUG 0
#include "debug.h"

static void _callback_unlock_mutex(void* arg)
{
    mutex_t *mutex = (mutex_t *) arg;
    mutex_unlock(mutex);
}

void _xtimer_sleep(uint32_t offset, uint32_t long_offset)
{
    if (inISR()) {
        assert(!long_offset);
        xtimer_spin(offset);
    }

    xtimer_t timer;
    mutex_t mutex = MUTEX_INIT;

    timer.callback = _callback_unlock_mutex;
    timer.arg = (void*) &mutex;

    mutex_lock(&mutex);
    _xtimer_set64(&timer, offset, long_offset);
    mutex_lock(&mutex);
}

void xtimer_usleep_until(uint32_t *last_wakeup, uint32_t interval) {
    xtimer_t timer;
    mutex_t mutex = MUTEX_INIT;

    timer.callback = _callback_unlock_mutex;
    timer.arg = (void*) &mutex;

    uint32_t target = *last_wakeup + interval;

    uint32_t now = xtimer_now();
    /* make sure we're not setting a value in the past */
    if (now < *last_wakeup) {
        /* base timer overflowed */
        if (!((target < *last_wakeup) && (target > now))) {
            goto out;
        }
    }
    else if (! ((target < *last_wakeup) || (target > now))) {
        goto out;
    }

    uint32_t offset = target - now;

    if (offset > XTIMER_BACKOFF+XTIMER_USLEEP_UNTIL_OVERHEAD+1) {
        mutex_lock(&mutex);
        _xtimer_set_absolute(&timer, target - XTIMER_USLEEP_UNTIL_OVERHEAD);
        mutex_lock(&mutex);
    }
    else {
        xtimer_spin_until(target);
    }

out:
    *last_wakeup = target;
}

static void _callback_msg(void* arg)
{
    msg_t *msg = (msg_t*)arg;
    msg_send_int(msg, msg->sender_pid);
}

static inline void _setup_msg(xtimer_t *timer, msg_t *msg, kernel_pid_t target_pid)
{
    timer->callback = _callback_msg;
    timer->arg = (void*) msg;

    /* use sender_pid field to get target_pid into callback function */
    msg->sender_pid = target_pid;
}

void xtimer_set_msg(xtimer_t *timer, uint32_t offset, msg_t *msg, kernel_pid_t target_pid)
{
    _setup_msg(timer, msg, target_pid);
    xtimer_set(timer, offset);
}

void xtimer_set_msg64(xtimer_t *timer, uint64_t offset, msg_t *msg, kernel_pid_t target_pid)
{
    _setup_msg(timer, msg, target_pid);
    _xtimer_set64(timer, offset, offset >> 32);
}

static void _callback_wakeup(void* arg)
{
    thread_wakeup((kernel_pid_t)((intptr_t)arg));
}

void xtimer_set_wakeup(xtimer_t *timer, uint32_t offset, kernel_pid_t pid)
{
    timer->callback = _callback_wakeup;
    timer->arg = (void*) ((intptr_t)pid);

    xtimer_set(timer, offset);
}

/**
 * see http://www.hackersdelight.org/magic.htm.
 * This is to avoid using long integer division functions
 * the compiler otherwise links in.
 */
static inline uint64_t _ms_to_sec(uint64_t ms)
{
    return (unsigned long long)(ms * 0x431bde83) >> (0x12 + 32);
}

void xtimer_now_timex(timex_t *out)
{
    uint64_t now = xtimer_now64();

    out->seconds = _ms_to_sec(now);
    out->microseconds = now - (out->seconds * SEC_IN_USEC);
}

int xtimer_msg_receive_timeout64(msg_t *m, uint64_t timeout) {
    msg_t tmsg;
    tmsg.type = MSG_XTIMER;
    tmsg.content.ptr = (char *) &tmsg;

    xtimer_t t;
    xtimer_set_msg64(&t, timeout, &tmsg, sched_active_pid);

    msg_receive(m);
    if (m->type == MSG_XTIMER && m->content.ptr == (char *) &tmsg) {
        /* we hit the timeout */
        return -1;
    }
    else {
        xtimer_remove(&t);
        return 1;
    }
}
