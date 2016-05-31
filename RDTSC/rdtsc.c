#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <inttypes.h>
#include "rdtsc.h"

uint64_t accum = 0;

#define TIMES 1000000000
#define MILLION 1000000

int main(void) {

	struct timeval tv_before, tv_after;
	int ret, i;

	ret = gettimeofday(&tv_before, NULL);
	if(ret) {
		perror("gettimeofday:");
		exit(-1);
	}

	for(i = 0; i < TIMES; i++) {
		accum += rdtsc();
	}

	ret = gettimeofday(&tv_after, NULL);
	if(ret) {
		perror("gettimeofday:");
		exit(-1);
	}

	printf("Result is %lld\n", accum);

	printf("Time per iteration: %.2f microsecond \n",
	       ((double)((tv_after.tv_sec - tv_before.tv_sec) * MILLION +
			 (tv_after.tv_usec - tv_before.tv_usec)))
	       /(double) TIMES);
}
