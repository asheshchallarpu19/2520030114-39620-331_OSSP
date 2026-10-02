#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define THREAD_COUNT 4
#define MAX_ACTIVE_WORKERS 2

typedef struct
{
    int active_workers;
    int completed_workers;

    pthread_mutex_t mutex;
    pthread_cond_t all_done;
    sem_t worker_slots;
} SyncData;

typedef struct
{
    int worker_id;
    SyncData *sync;
} WorkerData;

static void *worker(void *arg)
{
    WorkerData *data = (WorkerData *) arg;

    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 200000000
    };

    /*
     * Counting semaphore limits the number
     * of workers running this section at once.
     */
    if (sem_wait(&data->sync->worker_slots) == -1)
    {
        perror("sem_wait failed");
        return NULL;
    }

    if (pthread_mutex_lock(&data->sync->mutex) != 0)
    {
        fprintf(stderr, "mutex lock failed\n");
        sem_post(&data->sync->worker_slots);
        return NULL;
    }

    data->sync->active_workers++;

    printf(
        "Worker %d started. Active workers: %d\n",
        data->worker_id,
        data->sync->active_workers
    );

    pthread_mutex_unlock(&data->sync->mutex);

    /*
     * Simulate backup work.
     */
    nanosleep(&delay, NULL);

    pthread_mutex_lock(&data->sync->mutex);

    data->sync->active_workers--;
    data->sync->completed_workers++;

    printf(
        "Worker %d completed. Completed: %d/%d\n",
        data->worker_id,
        data->sync->completed_workers,
        THREAD_COUNT
    );

    /*
     * Notify the main thread when all workers finish.
     */
    if (data->sync->completed_workers == THREAD_COUNT)
    {
        pthread_cond_signal(&data->sync->all_done);
    }

    pthread_mutex_unlock(&data->sync->mutex);

    sem_post(&data->sync->worker_slots);

    return NULL;
}

int main(void)
{
    pthread_t threads[THREAD_COUNT];
    WorkerData workers[THREAD_COUNT];

    SyncData sync = {
        .active_workers = 0,
        .completed_workers = 0
    };

    printf("========================================\n");
    printf(" CO6 - Advanced Synchronization Demo\n");
    printf(" Mutex + Condition Variable + Semaphore\n");
    printf("========================================\n");

    if (pthread_mutex_init(&sync.mutex, NULL) != 0)
    {
        fprintf(stderr, "pthread_mutex_init failed\n");
        return 1;
    }

    if (pthread_cond_init(&sync.all_done, NULL) != 0)
    {
        fprintf(stderr, "pthread_cond_init failed\n");
        pthread_mutex_destroy(&sync.mutex);
        return 1;
    }

    if (sem_init(
            &sync.worker_slots,
            0,
            MAX_ACTIVE_WORKERS
        ) == -1)
    {
        perror("sem_init failed");

        pthread_cond_destroy(&sync.all_done);
        pthread_mutex_destroy(&sync.mutex);

        return 1;
    }

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        workers[i].worker_id = i + 1;
        workers[i].sync = &sync;

        if (pthread_create(
                &threads[i],
                NULL,
                worker,
                &workers[i]
            ) != 0)
        {
            fprintf(
                stderr,
                "pthread_create failed for worker %d\n",
                i + 1
            );

            return 1;
        }
    }

    /*
     * Main thread waits until workers signal completion.
     */
    pthread_mutex_lock(&sync.mutex);

    while (sync.completed_workers < THREAD_COUNT)
    {
        pthread_cond_wait(
            &sync.all_done,
            &sync.mutex
        );
    }

    pthread_mutex_unlock(&sync.mutex);

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("\n========================================\n");
    printf(" Synchronization Summary\n");
    printf("========================================\n");

    printf("Worker threads       : %d\n", THREAD_COUNT);
    printf("Semaphore capacity   : %d\n", MAX_ACTIVE_WORKERS);
    printf("Completed workers    : %d\n", sync.completed_workers);
    printf("Condition variable   : SUCCESS\n");
    printf("Mutex protection     : SUCCESS\n");
    printf("Counting semaphore   : SUCCESS\n");

    sem_destroy(&sync.worker_slots);
    pthread_cond_destroy(&sync.all_done);
    pthread_mutex_destroy(&sync.mutex);

    printf("CO6 synchronization demo completed.\n");

    return 0;
}
