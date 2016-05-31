#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <inttypes.h>
#include <pthread.h>

#define MILLION 1000000
#define MAX_THREADS 96
#define EXPERIMENT_DURATION_SECONDS 10

static unsigned ratio = 1;
static pthread_mutex_t mutex;

#define CS_DURATION 1
#define NON_CS_DURATION CS_DURATION * ratio

typedef struct {
	pthread_t tid;
	unsigned iters_completed;
} thread_data_t;

static void
init_lock(void) {

	pthread_mutex_init(&mutex, NULL);

}
static void
acquire_lock(void) {

	pthread_mutex_lock(&mutex);
}

static void
release_lock(void) {

	pthread_mutex_unlock(&mutex);

}

static void
get_time_or_exit(struct timeval *tv) {

	int ret;

	ret = gettimeofday(tv, NULL);
	if(ret) {
		perror("gettimeofday:");
		exit(-1);
	}
}

static void
work (uint64_t target_duration_us) {

	struct timeval tv_begin, tv_now;
	uint64_t duration_us;

	get_time_or_exit(&tv_begin);

	while (1) {
		get_time_or_exit(&tv_now);

		duration_us =
			(tv_now.tv_sec - tv_begin.tv_sec) * MILLION +
			(tv_now.tv_usec - tv_begin.tv_usec);

		if(duration_us > target_duration_us)
			break;
	}
}

/*
 * The threads keep alternating between critical and non-critical sections
 * for the desired duration of the experiment.
 */
static void *
thread_func(void *arg) {

	int i;
	struct timeval tv_begin, tv_now;

	get_time_or_exit(&tv_begin);

	for(i = 0; ; i++) {

		acquire_lock();
		{
			work(CS_DURATION);
		}
		release_lock();

		work(NON_CS_DURATION);

		get_time_or_exit(&tv_now);

		if(tv_now.tv_sec - tv_begin.tv_sec >
		   EXPERIMENT_DURATION_SECONDS)
			break;
	}

	((thread_data_t*)arg)->iters_completed = i;

	return 0;
}


int main(int argc, char **argv) {

	int i, threads = 8;
	unsigned total_iters_completed;
	struct timeval tv_begin, tv_end;
	thread_data_t *thread_data;

	if(argc > 1) {
		threads = atoi(argv[1]);
		if(threads < 1 || threads > MAX_THREADS) {
			fprintf(stderr, "Invalid number of threads\n");
			exit(-1);
		}
	}

	if(argc > 2)
		ratio = atoi(argv[2]);

	printf("Threads: %d\n", threads);
	printf("Ratio: %d\n", ratio);
	printf("Target duration: %d\n", EXPERIMENT_DURATION_SECONDS);

	/* Allocate an array where each thread will report
	 * the number of critical sections that it completed.
	 */
	thread_data = (thread_data_t *) malloc(sizeof(thread_data_t) * threads);
	if(thread_data == NULL) {
		perror("malloc");
		exit(-1);
	}

	init_lock();
	get_time_or_exit(&tv_begin);

	for(i = 0; i < threads; i++) {

		int ret = pthread_create(&thread_data[i].tid, NULL,
					 thread_func, &thread_data[i]);
		if(ret) {
			perror("pthread_create");
			exit(-1);
		}
	}

	for(i = 0; i < threads; i++) {

		int ret = pthread_join(thread_data[i].tid, 0);
		if(ret) {
			perror("pthread_join");
			exit(-1);
		}
		total_iters_completed += thread_data[i].iters_completed;
	}

	get_time_or_exit(&tv_end);

	unsigned long actual_duration = tv_end.tv_sec - tv_begin.tv_sec;
	printf("Actual duration: %ld\n", actual_duration);
	printf("Iterations completed: %d\n", total_iters_completed);
	printf("Iterations per second: %ld\n", total_iters_completed /
	       actual_duration);

}
