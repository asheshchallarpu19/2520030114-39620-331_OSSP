#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>

#include "worker_pool.h"
#include "file_ops.h"

typedef struct CopyJob
{
    char *source;
    char *destination;
    CopyJobType type;

    struct CopyJob *next;

} CopyJob;

struct WorkerPool
{
    pthread_t *threads;
    size_t thread_count;

    CopyJob *head;
    CopyJob *tail;

    size_t queued_jobs;
    size_t active_workers;

    int shutdown;

    pthread_mutex_t mutex;
    pthread_cond_t work_available;
    pthread_cond_t all_done;

    SyncStats *stats;

    /*
     * Worker-side errors are accumulated privately.
     * They are merged into SyncStats only after all
     * worker jobs have completed.
     */
    unsigned long worker_errors;
};

static void free_job(CopyJob *job)
{
    if (job == NULL)
    {
        return;
    }

    free(job->source);
    free(job->destination);
    free(job);
}

static void *worker_main(void *argument)
{
    WorkerPool *pool = argument;

    for (;;)
    {
        CopyJob *job;
        int copy_result;

        pthread_mutex_lock(&pool->mutex);

        while (
            pool->head == NULL &&
            !pool->shutdown
        )
        {
            pthread_cond_wait(
                &pool->work_available,
                &pool->mutex
            );
        }

        /*
         * Workers terminate only after shutdown has
         * been requested and no queued jobs remain.
         */
        if (
            pool->shutdown &&
            pool->head == NULL
        )
        {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        job = pool->head;

        pool->head = job->next;

        if (pool->head == NULL)
        {
            pool->tail = NULL;
        }

        pool->queued_jobs--;
        pool->active_workers++;

        pthread_mutex_unlock(&pool->mutex);

        /*
         * Real file copying occurs concurrently here.
         *
         * copy_file() performs the safe temporary-file,
         * fsync(), and atomic rename workflow.
         */
        copy_result = copy_file(
            job->source,
            job->destination
        );

        struct stat copied_stat;
        int stat_result = -1;

        if (copy_result == 0)
        {
            stat_result = stat(
                job->source,
                &copied_stat
            );
        }

        pthread_mutex_lock(&pool->mutex);

        if (copy_result == 0)
        {
            if (
                stat_result == 0 &&
                copied_stat.st_size > 0
            )
            {
                pool->stats->bytes_copied +=
                    (unsigned long long) copied_stat.st_size;
            }
            if (job->type == COPY_JOB_NEW)
            {
                printf(
                    "[COPY] %s (NEW)\n",
                    job->source
                );

                pool->stats->new_files++;
            }
            else
            {
                printf(
                    "[UPDATE] %s (MODIFIED)\n",
                    job->source
                );

                pool->stats->updated_files++;
            }
        }
        else
        {
            if (job->type == COPY_JOB_NEW)
            {
                fprintf(
                    stderr,
                    "[ERROR] Failed to copy new file: %s\n",
                    job->source
                );
            }
            else
            {
                fprintf(
                    stderr,
                    "[ERROR] Failed to update file: %s\n",
                    job->source
                );
            }

            pool->worker_errors++;
        }

        pool->active_workers--;

        /*
         * Wake worker_pool_wait() when there are
         * no queued jobs and no workers still copying.
         */
        if (
            pool->head == NULL &&
            pool->active_workers == 0
        )
        {
            pthread_cond_broadcast(
                &pool->all_done
            );
        }

        pthread_mutex_unlock(&pool->mutex);

        free_job(job);
    }

    return NULL;
}

WorkerPool *worker_pool_create(
    size_t thread_count,
    SyncStats *stats
)
{
    WorkerPool *pool;
    size_t created_threads = 0;
    int error;

    if (
        thread_count == 0 ||
        stats == NULL
    )
    {
        return NULL;
    }

    pool = calloc(
        1,
        sizeof(*pool)
    );

    if (pool == NULL)
    {
        perror("calloc worker pool failed");
        return NULL;
    }

    pool->threads = calloc(
        thread_count,
        sizeof(*pool->threads)
    );

    if (pool->threads == NULL)
    {
        perror("calloc worker threads failed");

        free(pool);
        return NULL;
    }

    pool->thread_count = thread_count;
    pool->stats = stats;

    error = pthread_mutex_init(
        &pool->mutex,
        NULL
    );

    if (error != 0)
    {
        fprintf(
            stderr,
            "pthread_mutex_init failed: %s\n",
            strerror(error)
        );

        free(pool->threads);
        free(pool);

        return NULL;
    }

    error = pthread_cond_init(
        &pool->work_available,
        NULL
    );

    if (error != 0)
    {
        fprintf(
            stderr,
            "pthread_cond_init failed: %s\n",
            strerror(error)
        );

        pthread_mutex_destroy(&pool->mutex);

        free(pool->threads);
        free(pool);

        return NULL;
    }

    error = pthread_cond_init(
        &pool->all_done,
        NULL
    );

    if (error != 0)
    {
        fprintf(
            stderr,
            "pthread_cond_init failed: %s\n",
            strerror(error)
        );

        pthread_cond_destroy(
            &pool->work_available
        );

        pthread_mutex_destroy(
            &pool->mutex
        );

        free(pool->threads);
        free(pool);

        return NULL;
    }

    for (
        created_threads = 0;
        created_threads < thread_count;
        created_threads++
    )
    {
        error = pthread_create(
            &pool->threads[created_threads],
            NULL,
            worker_main,
            pool
        );

        if (error != 0)
        {
            size_t i;

            fprintf(
                stderr,
                "pthread_create failed: %s\n",
                strerror(error)
            );

            pthread_mutex_lock(
                &pool->mutex
            );

            pool->shutdown = 1;

            pthread_cond_broadcast(
                &pool->work_available
            );

            pthread_mutex_unlock(
                &pool->mutex
            );

            for (
                i = 0;
                i < created_threads;
                i++
            )
            {
                pthread_join(
                    pool->threads[i],
                    NULL
                );
            }

            pthread_cond_destroy(
                &pool->all_done
            );

            pthread_cond_destroy(
                &pool->work_available
            );

            pthread_mutex_destroy(
                &pool->mutex
            );

            free(pool->threads);
            free(pool);

            return NULL;
        }
    }

    return pool;
}

int worker_pool_submit(
    WorkerPool *pool,
    const char *source,
    const char *destination,
    CopyJobType type
)
{
    CopyJob *job;

    if (
        pool == NULL ||
        source == NULL ||
        destination == NULL
    )
    {
        return -1;
    }

    job = calloc(
        1,
        sizeof(*job)
    );

    if (job == NULL)
    {
        perror("calloc copy job failed");
        return -1;
    }

    job->source = strdup(source);

    if (job->source == NULL)
    {
        perror("strdup source path failed");

        free_job(job);
        return -1;
    }

    job->destination = strdup(destination);

    if (job->destination == NULL)
    {
        perror("strdup destination path failed");

        free_job(job);
        return -1;
    }

    job->type = type;

    pthread_mutex_lock(
        &pool->mutex
    );

    if (pool->shutdown)
    {
        pthread_mutex_unlock(
            &pool->mutex
        );

        free_job(job);
        return -1;
    }

    if (pool->tail == NULL)
    {
        pool->head = job;
        pool->tail = job;
    }
    else
    {
        pool->tail->next = job;
        pool->tail = job;
    }

    pool->queued_jobs++;

    pthread_cond_signal(
        &pool->work_available
    );

    pthread_mutex_unlock(
        &pool->mutex
    );

    return 0;
}

int worker_pool_wait(
    WorkerPool *pool
)
{
    if (pool == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(
        &pool->mutex
    );

    while (
        pool->head != NULL ||
        pool->active_workers != 0
    )
    {
        pthread_cond_wait(
            &pool->all_done,
            &pool->mutex
        );
    }

    /*
     * At this point no worker is modifying worker_errors.
     * Merge worker failures into the main statistics safely.
     */
    if (pool->worker_errors != 0)
    {
        pool->stats->errors += pool->worker_errors;
        pool->worker_errors = 0;
    }

    pthread_mutex_unlock(
        &pool->mutex
    );

    return 0;
}

size_t worker_pool_thread_count(
    const WorkerPool *pool
)
{
    if (pool == NULL)
    {
        return 0;
    }

    return pool->thread_count;
}

void worker_pool_destroy(
    WorkerPool *pool
)
{
    size_t i;
    CopyJob *job;

    if (pool == NULL)
    {
        return;
    }

    /*
     * Finish all submitted copy operations before
     * shutting the worker threads down.
     */
    worker_pool_wait(pool);

    pthread_mutex_lock(
        &pool->mutex
    );

    pool->shutdown = 1;

    pthread_cond_broadcast(
        &pool->work_available
    );

    pthread_mutex_unlock(
        &pool->mutex
    );

    for (
        i = 0;
        i < pool->thread_count;
        i++
    )
    {
        pthread_join(
            pool->threads[i],
            NULL
        );
    }

    /*
     * Normally the queue is already empty because
     * worker_pool_wait() completed, but clean up any
     * remaining jobs defensively.
     */
    job = pool->head;

    while (job != NULL)
    {
        CopyJob *next = job->next;

        free_job(job);
        job = next;
    }

    pthread_cond_destroy(
        &pool->all_done
    );

    pthread_cond_destroy(
        &pool->work_available
    );

    pthread_mutex_destroy(
        &pool->mutex
    );

    free(pool->threads);
    free(pool);
}
