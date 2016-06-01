#ifndef __FAIRLOCK_H
#define __FAIRLOCK_H

#include <sys/types.h>
#include <inttypes.h>
#include <sys/time.h>

/*
 * A light weight lock that can be used to replace spinlocks if fairness is
 * necessary. Implements a ticket-based back off spin lock.
 * The fields are available as a union to allow for atomically setting
 * the state of the entire lock.
 */
struct __fair_lock {
	union {
		volatile uint32_t lock;
		struct {
			volatile uint16_t owner;  /* Ticket for current owner */
			volatile uint16_t waiter; /* Last allocated ticket */
		} s;
	} u;
#define	fair_lock_owner u.s.owner
#define	fair_lock_waiter u.s.waiter
};

typedef struct __fair_lock fair_lock_t;

int fair_lock(fair_lock_t *lock);
int fair_unlock(fair_lock_t *lock);
void fair_init(fair_lock_t *lock);

#endif
