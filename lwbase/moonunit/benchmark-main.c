#include <stdio.h>

#include "benchmark.h"

#define BUFFER_SIZE (64 * 1024)
#define SEND_SEGMENTS 1
#define NUM_ITERATIONS 2
#define NUM_PAIRS 5000

int main(int argc, char** argv)
{
    static BENCHMARK_SETTINGS settings =
    {
        .ulBufferSize = BUFFER_SIZE,
        .usSendSegments = SEND_SEGMENTS,
        .ulIterations = NUM_ITERATIONS,
        .ulPairs = NUM_PAIRS
    };
    PLW_THREAD_POOL pPool = NULL;
    ULONG64 ullTotal = 0;
    ULONG64 ullTime = 0;

    LwRtlCreateThreadPool(&pPool, NULL);

    BenchmarkThreadPool(
        pPool,
        &settings,
        &ullTime,
        &ullTotal);

    printf("Transferred %llu bytes in %.2f seconds, %.2f mbit/s\n",
           (unsigned long long) ullTotal,
           ullTime / 1000000000.0,
           (ullTotal / 131072.0) / (ullTime / 1000000000.0));

    return 0;
}
