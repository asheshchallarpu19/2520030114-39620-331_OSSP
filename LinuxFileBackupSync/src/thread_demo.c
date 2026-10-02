#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define THREAD_COUNT 4
#define FILES_PER_THREAD 5

typedef struct
{
    unsigned long files_processed;
    pthread_mutex_t mutex;
} SharedStats;

typedef struct
{
    int worker_id;
    SharedStats *stats;
} WorkerData;

static void *backup_worker(void *arg)
{
    WorkerData *data = (WorkerData *) arg;

    for (int i = 1; i <= FILES_PER_THREAD; i++)
    {
        /*
         * Multiple threads share files_processed.
         * The mutex prevents a race condition.
         */
        if (pthread_mutex_lock(&data->stats->mutex) != 0)
        {
            fprintf(stderr, "Worker %d: mutex lock failed\n",
                    data->worker_id);

            return NULL;
        }

        data->stats->files_processed++;

        printf(
            "Worker %d processed file %d "
            "(Total processed: %lu)\n",
            data->worker_id,
            i,
            data->stats->files_processed
        );

        if (pthread_mutex_unlock(&data->stats->mutex) != 0)
        {
            fprintf(stderr, "Worker %d: mutex unlock failed\n",
                    data->worker_id);

            return NULL;
        }
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[THREAD_COUNT];
    WorkerData workers[THREAD_COUNT];

    SharedStats stats = {
        .files_processed = 0
    };

    printf("========================================\n");
    printf(" POSIX Threads and Mutex Demonstration\n");
    printf("========================================\n");

    if (pthread_mutex_init(&stats.mutex, NULL) != 0)
    {
        fprintf(stderr, "pthread_mutex_init failed\n");
        return 1;
    }

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        workers[i].worker_id = i + 1;
        workers[i].stats = &stats;

        if (pthread_create(
                &threads[i],
                NULL,
                backup_worker,
                &workers[i]
            ) != 0)
        {
            fprintf(stderr,
                    "Failed to create worker thread %d\n",
                    i + 1);

            pthread_mutex_destroy(&stats.mutex);
            return 1;
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            fprintf(stderr,
                    "Failed to join worker thread %d\n",
                    i + 1);

            pthread_mutex_destroy(&stats.mutex);
            return 1;
        }
    }

    printf("\n========================================\n");
    printf(" Thread Synchronization Summary\n");
    printf("========================================\n");

    printf(
        "Threads created      : %d\n",
        THREAD_COUNT
    );

    printf(
        "Files per thread     : %d\n",
        FILES_PER_THREAD
    );

    printf(
        "Expected total files : %d\n",
        THREAD_COUNT * FILES_PER_THREAD
    );

    printf(
        "Actual total files   : %lu\n",
        stats.files_processed
    );

    if (stats.files_processed ==
        (unsigned long)(THREAD_COUNT * FILES_PER_THREAD))
    {
        printf("Result               : SUCCESS\n");
        printf("Shared data protected correctly by mutex.\n");
    }
    else
    {
        printf("Result               : ERROR\n");
    }

    if (pthread_mutex_destroy(&stats.mutex) != 0)
    {
        fprintf(stderr, "pthread_mutex_destroy failed\n");
        return 1;
    }

    return 0;
}
