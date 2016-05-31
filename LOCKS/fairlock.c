#include <fairlock.h>

#define SPINCOUNT 1000
#define MILLION 1000000

static void
__sleep(uint64_t seconds, uint64_t micro_seconds)
{
	struct timeval t;

	t.tv_sec = (time_t)(seconds + micro_seconds / MILLION);
	t.tv_usec = (suseconds_t)(micro_seconds % MILLION);

	(void)select(0, NULL, NULL, NULL, &t);
}

void
fair_init(fair_lock_t *lock) {
	lock->u.lock = 0;
}

/*
 * fair_lock --
 *	Get a lock.
 */
int
fair_lock(fair_lock_t *lock)
{
	uint16_t ticket;
	int pause_cnt;

	/*
	 * Possibly wrap: if we have more than 64K lockers waiting, the ticket
	 * value will wrap and two lockers will simultaneously be granted the
	 * lock.
	 */
	ticket = __atomic_fetch_add(&lock->fair_lock_waiter, 1,
				      __ATOMIC_SEQ_CST);

	for (pause_cnt = 0; ticket != lock->fair_lock_owner;) {
		/*
		 * We failed to get the lock; pause before retrying and if we've
		 * paused enough, sleep so we don't burn CPU to no purpose. This
		 * situation happens if there are more threads than cores in the
		 * system and we're thrashing on shared resources.
		 */

		if (++pause_cnt < SPINCOUNT)
			;
		else
			__sleep(0, 10);
	}

	/*
	 * Applications depend on a barrier here so that operations holding the
	 * lock see consistent data.
	 */
	__sync_synchronize();

	return (0);
}

/*
 * fair_unlock --
 *	Release a shared lock.
 */
int
fair_unlock(fair_lock_t *lock)
{

	/*
	 * Ensure that all updates made while the lock was held are visible to
	 * the next thread to acquire the lock.
	 */
	__sync_synchronize();

	/*
	 * We have exclusive access - the update does not need to be atomic.
	 */
	++lock->fair_lock_owner;

	return (0);
}
