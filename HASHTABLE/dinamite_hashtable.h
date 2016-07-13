#ifndef DINAMITE_HASHTABLE_H
#define DINAMITE_HASHTABLE_H

#include <sys/types.h>
#include <inttypes.h>

/*
 * A hashtable with per-thread paritions and dynamically sized buckets.
 * The hashtable behaves like a set: duplicate items are discarded.
 * The hashtable stores unsigned 64-bit values.
 */
void dinamite_hashtable_put(uint64_t value, int threadID);
void dinamite_hashtable_begin_iterate(int threadID);
int dinamite_hashtable_getnext(int threadID, uint64_t *value_ptr);
void dinamite_hashtable_clear(void);

#endif
