#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dinamite_hashtable.h"


void test1(void *tid) {

#define ITEMS 1024
	int threadID = (int)tid;

	if( threadID == 0 )
		printf("Starting Test 1...\n");

	for(int i = 1; i <= ITEMS; i++)
		dinamite_hashtable_put(i, threadID);

	dinamite_hashtable_begin_iterate(threadID);

	for(int i = 1; i <= ITEMS; i++) {
		uint64_t value;

		if(dinamite_hashtable_getnext(threadID, &value) != 0) {
			printf("test1: could not retrieve item %d\n", i);
			return;
		}
		if(value != i) {
			printf("test1: value error: got %d, expecting %d\n",
			       (int)value, i);
			return;
		}
	}

	if( (int)threadID == 0 )
		printf("Done.\n");
}

void test2(void *tid) {

#define ITEMS 1024
#define MULTIPLIER 1024
	int threadID = (int)tid;

	if( threadID == 0 )
		printf("Starting Test 2...\n");

	for(int i = 1; i <= ITEMS; i++)
		dinamite_hashtable_put(i * MULTIPLIER, threadID);

	dinamite_hashtable_begin_iterate(threadID);

	for(uint64_t i = 1; i <= ITEMS; i++) {
		uint64_t value;

		if(dinamite_hashtable_getnext(threadID, &value) != 0) {
			printf("test2: could not retrieve item %lld\n", i);
			return;
		}
		if(value != i * MULTIPLIER) {
			printf("test2: value error: got %lld, expecting %lld\n",
			       value, i * MULTIPLIER);
			return;
		}
	}

	if( threadID == 0 )
		printf("Done.\n");

}

void test3(void) {

#define NUM_THREADS 127
	pthread_t threads[NUM_THREADS];

	printf("Starting Test 3...\n");
	for( int i = 0; i < NUM_THREADS; i++ ) {
		int ret = pthread_create(&threads[i], NULL,
					 (void *(*)(void *)) test1,
					 (void*)(i+1));
		if(ret != 0) {
			fprintf(stderr, "pthread_create: %s\n", strerror(ret));
			exit(-1);
		}
	}

	for( int i = 0; i < NUM_THREADS; i++ ) {
		int ret = pthread_join(threads[i], NULL);
		if(ret != 0) {
			fprintf(stderr, "pthread_join: %s\n", strerror(ret));
			exit(-1);
		}
	}

	printf("Done.\n");
}

void test4(void) {

#define NUM_THREADS 127
	pthread_t threads[NUM_THREADS];

	printf("Starting Test 4...\n");
	for( int i = 0; i < NUM_THREADS; i++ ) {
		int ret = pthread_create(&threads[i], NULL,
					 (void *(*)(void *)) test2,
					 (void*)(i+1));
		if(ret != 0) {
			fprintf(stderr, "pthread_create: %s\n", strerror(ret));
			exit(-1);
		}
	}

	for( int i = 0; i < NUM_THREADS; i++ ) {
		int ret = pthread_join(threads[i], NULL);
		if(ret != 0) {
			fprintf(stderr, "pthread_join: %s\n", strerror(ret));
			exit(-1);
		}
	}

	printf("Done.\n");
}

int main(void) {

	test1(0);
	dinamite_hashtable_clear();
	test2(0);
	dinamite_hashtable_clear();
	test3();
	dinamite_hashtable_clear();
	test4();
}
