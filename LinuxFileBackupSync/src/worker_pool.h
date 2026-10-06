#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <stddef.h>

#include "sync.h"

/*
 * Describes why a file-copy job was submitted.
 */
typedef enum
{
    COPY_JOB_NEW,
    COPY_JOB_MODIFIED
} CopyJobType;

/*
 * WorkerPool is intentionally opaque.
 *
 * Its internal queue, mutexes, condition variables,
 * and pthread objects are hidden inside worker_pool.c.
 */
typedef struct WorkerPool WorkerPool;

/*
 * Create a worker pool.
 *
 * thread_count:
 *     Number of worker threads to create.
 *
 * stats:
 *     Shared synchronization statistics. Updates performed
 *     by worker threads will be protected internally.
 *
 * Returns:
 *     Pointer to WorkerPool on success.
 *     NULL on failure.
 */
WorkerPool *worker_pool_create(
    size_t thread_count,
    SyncStats *stats
);

/*
 * Submit one real file-copy operation.
 *
 * The worker pool makes its own copy of both path strings,
 * so the caller may safely reuse its local path buffers.
 *
 * Returns:
 *     0 on success
 *    -1 on failure
 */
int worker_pool_submit(
    WorkerPool *pool,
    const char *source,
    const char *destination,
    CopyJobType type
);

/*
 * Wait until:
 *
 * - the job queue is empty, and
 * - all currently active workers have finished.
 *
 * Returns:
 *     0 if all queued work has completed.
 *    -1 if the pool is invalid.
 */
int worker_pool_wait(
    WorkerPool *pool
);

/*
 * Return the number of worker threads in the pool.
 */
size_t worker_pool_thread_count(
    const WorkerPool *pool
);

/*
 * Stop all workers, join their threads,
 * release queued jobs, destroy synchronization
 * primitives, and free the pool.
 */
void worker_pool_destroy(
    WorkerPool *pool
);

#endif
