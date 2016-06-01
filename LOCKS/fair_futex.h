#ifndef __FAIR_FUTEX_H
#define __FAIR_FUTEX_H

#include <linux/futex.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fairlock.h>

typedef struct __fair_futex {
	fair_lock_t fairlock;
	volatile uint16_t futex;
} fair_futex_t;

void fair_futex_init(fair_futex_t *lock);
int fair_futex_lock(fair_futex_t *lock);
int fair_futex_unlock(fair_futex_t *lock);

#endif
