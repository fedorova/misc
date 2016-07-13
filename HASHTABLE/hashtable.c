


#define MAX_THREADS 128
#define HT_NUMBUCKETS 512
#define INIT_BUCKET_SIZE 32

static int hashtable_numbuckets;

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
} dinamite_bucket_t;

typedef struct __hashtable {
	dinamite_bucket_t ht_buckets[HT_NUMBUCKETS];
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
	     = (dinamite_hashtable_t *)
	     malloc(sizeof(dinamite_hashtable_t)) == NULL)) {
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
			"not be allocated. \n");
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
	uint32_t hash;

	if(__dinamite_ht_checkinit(threadID) != 0)
		return;

	hash = (((uint32_t)value & 0xFFFFF000 ) >> 12) % HT_NUMBUCKETS;
	bucket = &per_thread_hashtable[threadID][hash];

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
	else if(bucket->bct_num_entries == bucket->bct_max_entries) {

		/* Reallocate */
		fprintf(stderr, "Reallocating the bucket array for thread %d "
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
	bucket->bct_entries[bucket->bct_num_entries++] = value;
}



