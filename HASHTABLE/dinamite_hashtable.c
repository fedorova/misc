#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dinamite_hashtable.h"

#define MAX_THREADS 128
#define HT_NUMBUCKETS 512
#define INIT_BUCKET_SIZE 32

/* Each hashtable bucket is a dynamic array. The number of entries tells us
 * how many entries are actually there; max entries tells us the maximum that
 * we can have. When the number of entries exceeds the maximum, we reallocate
 * the entries array doubling its size and copying the content of the old array
 * into the new one.
 */
typedef struct __bucket {
	int bct_num_entries;
	int bct_max_entries;
	uint64_t *bct_entries;
	unsigned bct_iter_marker;
} dinamite_bucket_t;

typedef struct __hashtable {
	dinamite_bucket_t ht_buckets[HT_NUMBUCKETS];
	unsigned ht_iter_marker;
} dinamite_hashtable_t;

dinamite_hashtable_t *per_thread_hashtables[MAX_THREADS];

/* A hashtable that stores 64-bit values. There is a hashtable per thread.
 * We assume that thread IDs are small monotonically increasing numbers.
 * For simplicity and to limit the overhead, we assume the limit on the number
 * of threads. The limit can be easily changed by replacing the corresponding
 * #define.
 * On failure, either because the thread ID does not match our expectations
 * or if malloc fails, we report the problem to stdout, but do not crash.
 * We simply avoid future accesses to the broken hashtable.
 */
static int
__dinamite_ht_init(int threadID) {

	if( (per_thread_hashtables[threadID]
	     = (dinamite_hashtable_t *) malloc(sizeof(dinamite_hashtable_t)))
	    == NULL) {
		fprintf(stderr, "Warning: malloc() returned NULL when "
			"allocating a hashtable for threadID %d\n",
			threadID);
		return -1;
	}
	else
		memset(per_thread_hashtables[threadID], 0,
		       sizeof(dinamite_hashtable_t));
	return 0;
}

inline int __dinamite_ht_checkinit(threadID) {

	if( threadID < 0 || threadID > MAX_THREADS - 1 ) {
		fprintf(stderr, "Warning: threadID %d is greater than "
			"the MAX_THREADS value of %d. Hashtable could "
			"not be allocated. \n", threadID, MAX_THREADS);
		return -1;
	}

	if( per_thread_hashtables[threadID] == 0 )
		if(__dinamite_ht_init(threadID) != 0)
			return -1;
		else
			return 0;
	else
		return 0;
}

/* Hash into a bucket. If it is full, just go through the array until we find
 * the next empty one. Wrap around. If we run out of space, double the size
 * of the hash table and rehash.
 *
 * A bucket is empty if its corresponding value is zero.
 * This hashtable is used for storing 64-bit values that are pointers, so we
 * can safely assume that zero is an invalid value here.
 *
 * If we need to allocate the memory but fail, we report a warning, but continue
 * running, so that the trace can be recorded at least partially.
 */
void
dinamite_hashtable_put(uint64_t value, int threadID) {

	dinamite_bucket_t *bucket = NULL;
	uint32_t hash, i;

	if(__dinamite_ht_checkinit(threadID) != 0)
		return;

	hash = (((uint32_t)value & 0xFFFFF000 ) >> 12) % HT_NUMBUCKETS;
	bucket = &(per_thread_hashtables[threadID]->ht_buckets[hash]);

	/* Check if this bucket has not yet been allocated */
	if(bucket->bct_entries == NULL) {
		bucket->bct_entries =
			(uint64_t *)
			malloc(sizeof(uint64_t) * INIT_BUCKET_SIZE);
		if(bucket->bct_entries == NULL) {
			fprintf(stderr, "Warning: failed to allocate a "
				" bucket array for thread %d\n", threadID);
			return;
		}
		else
			bucket->bct_max_entries = INIT_BUCKET_SIZE;
	}
	/* Check if we have run out of space in the bucket */
	else if(bucket->bct_num_entries == bucket->bct_max_entries) {

		/* Reallocate */
		fprintf(stderr, "\tReallocating the bucket array for thread %d "
			"to %d entries.\n", threadID,
			bucket->bct_max_entries * 2);

		uint64_t *new_entries = (uint64_t*)
			malloc(sizeof(uint64_t) * bucket->bct_max_entries * 2);

		if(new_entries == NULL) {
			fprintf(stderr, "Warning: failed to allocate a "
				" bucket array for thread %d\n", threadID);
			return;
		}
		else {
			memset(new_entries, 0,
			       sizeof(uint64_t) * bucket->bct_max_entries * 2);
			memcpy(new_entries, bucket->bct_entries,
			       sizeof(uint64_t) * bucket->bct_num_entries);
			free(bucket->bct_entries);
			bucket->bct_entries = new_entries;
			bucket->bct_max_entries = bucket->bct_max_entries * 2;
		}
	}

	/* Check if this value is already in the bucket */
	for( i = 0; i < bucket->bct_num_entries; i++ )
		if(bucket->bct_entries[i] == value)
			return;

	/* The value is not there. Add it. */
	bucket->bct_entries[bucket->bct_num_entries++] = value;
}

/*
 * A call to this function resets the iteration markers for the hashtable.
 * This function must be called every time we begin iterating the hashtable
 * to ensure the iteration commences from the first item.
 */
void
dinamite_hashtable_begin_iterate(int threadID) {

	unsigned i;
	if(__dinamite_ht_checkinit(threadID) != 0)
		return;
	per_thread_hashtables[threadID]->ht_iter_marker = 0;

	for( i = 0; i < HT_NUMBUCKETS; i++ )
		per_thread_hashtables[threadID]->ht_buckets[i].bct_iter_marker
			= 0;
}

/*
 * This function returns the next item in the hashtable. The position of
 * the iteration is remembered by the iteration markers at the level of
 * the hashtable (to remember at what bucket we left of) and at the level
 * of the bucket.
 */
int dinamite_hashtable_getnext(int threadID, uint64_t *value_ptr) {

	dinamite_bucket_t * bucket;
	unsigned ht_marker, bct_marker;

	if(__dinamite_ht_checkinit(threadID) != 0)
		return -1;

encore:
	ht_marker = per_thread_hashtables[threadID]->ht_iter_marker;
	if(ht_marker > HT_NUMBUCKETS - 1)
		return -1;

	bucket = &per_thread_hashtables[threadID]->ht_buckets[ht_marker];

	if(bucket->bct_entries == NULL) {
		per_thread_hashtables[threadID]->ht_iter_marker++;
		goto encore;
	}
	else {
		if(bucket->bct_iter_marker == bucket->bct_num_entries) {
			per_thread_hashtables[threadID]->ht_iter_marker++;
			goto encore;
		}
		else {
			*value_ptr =
				bucket->bct_entries[bucket->bct_iter_marker++];
			return 0;
		}
	}
}

void dinamite_hashtable_clear(void) {

	for(int i = 0; i < MAX_THREADS; i++)
	{
		dinamite_hashtable_t *cur_ht;
		if( (cur_ht = per_thread_hashtables[i]) == NULL)
			continue;

		for(int j = 0; j < HT_NUMBUCKETS; j++) {
			if(cur_ht->ht_buckets[j].bct_entries == NULL)
				continue;
			else
				free(cur_ht->ht_buckets[j].bct_entries);
		}

		free(cur_ht);
		per_thread_hashtables[i] = 0;
	}
}
