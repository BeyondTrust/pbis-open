#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define THREADS_NUM         32
#define ALLOCS_PER_THREAD   1000000     // Number of allocations each thread will do per allocation size

static const size_t alloc_sizes[] = {512, 4*1024, 16*1024, 64*1024, 68*1024, 128*1024, 256*1024, 1024*1024};

static
void*
ThreadMain(
    void* p
    )
{
    void* mem = NULL;
    int i = 0;
    size_t alloc_size = *((size_t*)p);

    for (i = 0; i < ALLOCS_PER_THREAD; ++i)
    {
        mem = malloc(alloc_size);
        free(mem);
    }

    return NULL;
}

int
main(int argc, char** argv)
{
    int i = 0;
    int j = 0;
    int err = 0;
    pthread_t threads[THREADS_NUM] = { 0 };

    for (j = 0; j < sizeof(alloc_sizes)/sizeof(*alloc_sizes); ++j)
    {
        struct timespec start_time = { 0 };
        struct timespec end_time = { 0 };
        long time_diff_nsec = 0;

        // Create threads
        for (i = 0; i < THREADS_NUM; ++i)
        {
            if ((err = pthread_create(&threads[i], NULL, ThreadMain, (void*)&alloc_sizes[j])) != 0)
            {
                fprintf(stderr, "ERROR: %d\n", err);
                exit(1);
            }
        }

        if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time) == -1)
        {
            fprintf(stderr, "ERROR: %m");
            exit(1);
        }

        // Wait for threads termination
        for (i = 0; i < THREADS_NUM; ++i)
        {
            if ((err = pthread_join(threads[i], NULL)) != 0)
            {
                fprintf(stderr, "ERROR: %d\n", err);
                exit(1);
            }
        }

        if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time) == -1)
        {
            fprintf(stderr, "ERROR: %m");
            exit(1);
        }

        // Print results
        time_diff_nsec = (end_time.tv_sec - start_time.tv_sec) * 1000000000 + 
                         (end_time.tv_nsec - start_time.tv_nsec);

        printf("Allocations for %.7u bytes took \t%f(ms) per allocation\n", alloc_sizes[j], 
            time_diff_nsec / (double)1000000 / (double)ALLOCS_PER_THREAD / (double)THREADS_NUM);
    }

    return 0;
}
