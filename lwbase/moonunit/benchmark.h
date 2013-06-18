#include <lw/base.h>

typedef struct _BENCHMARK_SETTINGS
{
    ULONG ulBufferSize;
    USHORT usSendSegments;
    ULONG ulIterations;
    ULONG ulPairs;
} BENCHMARK_SETTINGS, *PBENCHMARK_SETTINGS;


VOID
BenchmarkThreadPool(
    PLW_THREAD_POOL pPool,
    PBENCHMARK_SETTINGS pSettings,
    PULONG64 pullDuration,
    PULONG64 pullBytesTransferred
    );
