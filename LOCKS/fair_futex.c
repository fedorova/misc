#include <fair_futex.h>
#include <limits.h>
#include <stdio.h>

static long
sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2,
int val2) {
	return syscall(SYS_futex, addr1, op, val2, timeout, addr2, val2);
}

#define SPIN_CONTROL 8

void
fair_futex_init(fair_futex_t *lock) {
	lock->futex = 0;
	fair_init(&lock->fairlock);
}

int
fair_futex_lock(fair_futex_t *lock) {

	uint32_t ticket;
	uint32_t old_futex;
	int pause_cnt;

	/*
	 * Possibly wrap: if we have more than 64K lockers waiting, the ticket
	 * value will wrap and two lockers will simultaneously be granted the
	 * lock.
	 */
	ticket = __atomic_fetch_add(&lock->fairlock.fair_lock_waiter, 1,
				      __ATOMIC_SEQ_CST);
retry:
	__sync_synchronize();
	old_futex = lock->futex;
	if(old_futex == (uint32_t)ticket / SPIN_CONTROL) {
//		printf("ticket %d spins (lo: %d)\n", ticket,
//		       lock->fairlock.fair_lock_owner);
		while (ticket != lock->fairlock.fair_lock_owner) ;
	}
	else {
//		printf("ticket %d sleeps (lo: %d)\n", ticket,
//		       lock->fairlock.fair_lock_owner);
		sys_futex((void*)&lock->futex, FUTEX_WAIT, old_futex, 0, 0, 0);
		goto retry;
	}

	/*
	 * Applications depend on a barrier here so that operations holding the
	 * lock see consistent data.
	 */
	__sync_synchronize();

//	printf("ticket %d got lock\n", ticket);

	return ticket;
}

int
fair_futex_unlock(fair_futex_t *lock) {

//	printf("ticket %d unlocking\n", lock->fairlock.fair_lock_owner);

	/* Only the lock holder ever increments the futex value */
	uint32_t old_futex = lock->futex;

	lock->futex = (lock->fairlock.fair_lock_owner + 1) / SPIN_CONTROL;
	__sync_synchronize();

	/* Only wake if we are changing the value of the futex */
	if(lock->futex != old_futex) {
//		printf("ticket %d to wake\n",  lock->fairlock.fair_lock_owner);
		sys_futex((void*)&lock->futex, FUTEX_WAKE, INT_MAX, 0, 0, 0);
//		printf("ticket %d wakes: old fut: %d, fut: %d\n",
//		       lock->fairlock.fair_lock_owner, old_futex, lock->futex);
	}
	else {
//		printf("DISASTER! old fut: %d, fut: %d flo: %d\n",
//		       old_futex, lock->futex, lock->fairlock.fair_lock_owner);
//		_exit(-1);
	}
	fair_unlock(&lock->fairlock);
}
