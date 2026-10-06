#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "sync.h"
#include "worker_pool.h"

#define DEFAULT_WORKER_THREADS 4
#define MAX_WORKER_THREADS 16

static int parse_thread_count(
    const char *value,
    size_t *thread_count
)
{
    char *end = NULL;
    long parsed;

    if (value == NULL || thread_count == NULL)
    {
        return -1;
    }

    errno = 0;

    parsed = strtol(value, &end, 10);

    if (
        errno != 0 ||
        end == value ||
        *end != '\0' ||
        parsed < 1 ||
        parsed > MAX_WORKER_THREADS
    )
    {
        return -1;
    }

    *thread_count = (size_t) parsed;

    return 0;
}

static int validate_source_directory(const char *path)
{
    struct stat st;

    if (stat(path, &st) == -1)
    {
        fprintf(
            stderr,
            "Source directory error for %s: %s\n",
            path,
            strerror(errno)
        );

        return -1;
    }

    if (!S_ISDIR(st.st_mode))
    {
        fprintf(
            stderr,
            "Source is not a directory: %s\n",
            path
        );

        return -1;
    }

    return 0;
}

static int path_is_child_of(
    const char *child,
    const char *parent
)
{
    size_t parent_length;

    if (
        child == NULL ||
        parent == NULL
    )
    {
        return 0;
    }

    /*
     * Every absolute path except "/" itself is below "/".
     */
    if (strcmp(parent, "/") == 0)
    {
        return (
            child[0] == '/' &&
            strcmp(child, "/") != 0
        );
    }

    parent_length = strlen(parent);

    return (
        strncmp(
            child,
            parent,
            parent_length
        ) == 0 &&
        child[parent_length] == '/'
    );
}

static int validate_path_relationship(
    const char *source,
    const char *backup
)
{
    char source_real[PATH_MAX];
    char backup_real[PATH_MAX];

    if (realpath(source, source_real) == NULL)
    {
        fprintf(
            stderr,
            "[ERROR] Cannot resolve source path %s: %s\n",
            source,
            strerror(errno)
        );

        return -1;
    }

    if (realpath(backup, backup_real) == NULL)
    {
        fprintf(
            stderr,
            "[ERROR] Cannot resolve backup path %s: %s\n",
            backup,
            strerror(errno)
        );

        return -1;
    }

    if (strcmp(source_real, backup_real) == 0)
    {
        fprintf(
            stderr,
            "[ERROR] Source and backup directories must be different.\n"
            "        Source: %s\n"
            "        Backup: %s\n",
            source_real,
            backup_real
        );

        return -1;
    }

    if (
        path_is_child_of(
            backup_real,
            source_real
        )
    )
    {
        fprintf(
            stderr,
            "[ERROR] Backup directory cannot be inside the source directory.\n"
            "        Source: %s\n"
            "        Backup: %s\n",
            source_real,
            backup_real
        );

        return -1;
    }

    if (
        path_is_child_of(
            source_real,
            backup_real
        )
    )
    {
        fprintf(
            stderr,
            "[ERROR] Source directory cannot be inside the backup directory.\n"
            "        Source: %s\n"
            "        Backup: %s\n",
            source_real,
            backup_real
        );

        return -1;
    }

    return 0;
}

static int ensure_backup_root(const char *path)
{
    struct stat st;

    if (stat(path, &st) == 0)
    {
        if (!S_ISDIR(st.st_mode))
        {
            fprintf(
                stderr,
                "Backup path exists but is not a directory: %s\n",
                path
            );

            return -1;
        }

        return 0;
    }

    if (errno != ENOENT)
    {
        fprintf(
            stderr,
            "Cannot access backup directory %s: %s\n",
            path,
            strerror(errno)
        );

        return -1;
    }

    if (mkdir(path, 0755) == -1)
    {
        if (errno == EEXIST)
        {
            if (
                stat(path, &st) == 0 &&
                S_ISDIR(st.st_mode)
            )
            {
                return 0;
            }
        }

        fprintf(
            stderr,
            "Cannot create backup directory %s: %s\n",
            path,
            strerror(errno)
        );

        return -1;
    }

    printf(
        "[MKDIR] Backup root created: %s\n",
        path
    );

    return 0;
}

static int acquire_backup_lock(const char *backup_dir)
{
    char lock_path[PATH_MAX];
    int lock_fd;
    int result;

    result = snprintf(
        lock_path,
        sizeof(lock_path),
        "%s/.backup_sync.lock",
        backup_dir
    );

    if (
        result < 0 ||
        (size_t) result >= sizeof(lock_path)
    )
    {
        fprintf(
            stderr,
            "[ERROR] Backup lock path is too long\n"
        );

        return -1;
    }

    lock_fd = open(
        lock_path,
        O_CREAT | O_RDWR,
        0644
    );

    if (lock_fd == -1)
    {
        fprintf(
            stderr,
            "[ERROR] Cannot open backup lock %s: %s\n",
            lock_path,
            strerror(errno)
        );

        return -1;
    }

    if (
        flock(
            lock_fd,
            LOCK_EX | LOCK_NB
        ) == -1
    )
    {
        if (
            errno == EWOULDBLOCK ||
            errno == EAGAIN
        )
        {
            fprintf(
                stderr,
                "[ERROR] Another backup process is already using:\n"
                "        %s\n",
                backup_dir
            );
        }
        else
        {
            fprintf(
                stderr,
                "[ERROR] Cannot lock backup directory %s: %s\n",
                backup_dir,
                strerror(errno)
            );
        }

        close(lock_fd);
        return -1;
    }

    printf(
        "[LOCK] Exclusive backup lock acquired\n"
    );

    return lock_fd;
}

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;
    int pipe_fd[2];
    int lock_fd;

    size_t thread_count = DEFAULT_WORKER_THREADS;
    int delete_enabled = 0;

    char message[64] = {0};

    int i;

    if (argc < 3)
    {
        fprintf(
            stderr,
            "Usage: %s <source_directory> <backup_directory> "
            "[--threads N] [--delete]\n",
            argv[0]
        );

        return 1;
    }

    /*
     * Parse optional arguments after source and backup.
     *
     * Supported:
     *   --threads N
     *   --delete
     *
     * Options may appear in either order.
     */
    for (i = 3; i < argc; i++)
    {
        if (strcmp(argv[i], "--delete") == 0)
        {
            if (delete_enabled)
            {
                fprintf(
                    stderr,
                    "Duplicate option: --delete\n"
                );

                return 1;
            }

            delete_enabled = 1;
        }
        else if (strcmp(argv[i], "--threads") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(
                    stderr,
                    "Missing value after --threads\n"
                );

                return 1;
            }

            if (
                parse_thread_count(
                    argv[i + 1],
                    &thread_count
                ) == -1
            )
            {
                fprintf(
                    stderr,
                    "Invalid thread count: %s\n"
                    "Thread count must be between 1 and %d.\n",
                    argv[i + 1],
                    MAX_WORKER_THREADS
                );

                return 1;
            }

            i++;
        }
        else
        {
            fprintf(
                stderr,
                "Unknown option: %s\n",
                argv[i]
            );

            fprintf(
                stderr,
                "Usage: %s <source_directory> <backup_directory> "
                "[--threads N] [--delete]\n",
                argv[0]
            );

            return 1;
        }
    }

    if (validate_source_directory(argv[1]) == -1)
    {
        return 1;
    }

    if (ensure_backup_root(argv[2]) == -1)
    {
        return 1;
    }

    if (
        validate_path_relationship(
            argv[1],
            argv[2]
        ) == -1
    )
    {
        return 1;
    }

    lock_fd = acquire_backup_lock(argv[2]);

    if (lock_fd == -1)
    {
        return 1;
    }

    printf("========================================\n");
    printf(" Linux File Backup & Synchronization\n");
    printf("========================================\n");

    printf("Source : %s\n", argv[1]);
    printf("Backup : %s\n", argv[2]);
    printf("Threads: %zu\n", thread_count);
    printf(
        "Delete : %s\n",
        delete_enabled ? "ENABLED" : "DISABLED"
    );
    printf("Main Process PID: %d\n", getpid());

    fflush(NULL);

    if (pipe(pipe_fd) == -1)
    {
        perror("pipe failed");

        close(lock_fd);

        return 1;
    }

    pid = fork();

    if (pid < 0)
    {
        perror("fork failed");

        close(pipe_fd[0]);
        close(pipe_fd[1]);
        close(lock_fd);

        return 1;
    }

    if (pid == 0)
    {
        SyncStats stats = {0};
        WorkerPool *pool;
        int result = 0;

        struct timespec start_time;
        struct timespec end_time;

        clock_gettime(
            CLOCK_MONOTONIC,
            &start_time
        );

        close(pipe_fd[0]);

        printf(
            "\n--- Child Process: Synchronization Worker ---\n"
        );

        /*
         * In mirror mode remove entries that no longer
         * exist in the source BEFORE synchronization.
         *
         * Doing this first also handles file/directory
         * type changes safely.
         */
        if (delete_enabled)
        {
            printf(
                "[DELETE-MODE] Mirror deletion enabled\n"
            );

            if (
                delete_extraneous_entries(
                    argv[1],
                    argv[2],
                    &stats
                ) == -1
            )
            {
                result = -1;
            }
        }

        pool = worker_pool_create(
            thread_count,
            &stats
        );

        if (pool == NULL)
        {
            fprintf(
                stderr,
                "[ERROR] Failed to create backup worker pool\n"
            );

            stats.errors++;
            result = -1;
        }
        else
        {
            stats.worker_threads =
                (unsigned long) worker_pool_thread_count(pool);

            printf(
                "[THREADS] Worker pool started with %zu threads\n",
                worker_pool_thread_count(pool)
            );

            if (
                sync_directory(
                    argv[1],
                    argv[2],
                    &stats,
                    pool
                ) == -1
            )
            {
                result = -1;
            }

            /*
             * Wait for every queued NEW/MODIFIED copy.
             */
            if (worker_pool_wait(pool) == -1)
            {
                fprintf(
                    stderr,
                    "[ERROR] Failed while waiting for worker pool\n"
                );

                stats.errors++;
                result = -1;
            }

            worker_pool_destroy(pool);
        }

        clock_gettime(
            CLOCK_MONOTONIC,
            &end_time
        );

        stats.elapsed_seconds =
            (double)(end_time.tv_sec - start_time.tv_sec) +
            (double)(end_time.tv_nsec - start_time.tv_nsec) /
            1000000000.0;

        print_sync_summary(&stats);

        if (
            result == 0 &&
            stats.errors == 0
        )
        {
            strcpy(
                message,
                "SYNC_SUCCESS"
            );
        }
        else
        {
            strcpy(
                message,
                "SYNC_COMPLETED_WITH_ERRORS"
            );
        }

        if (
            write(
                pipe_fd[1],
                message,
                strlen(message) + 1
            ) == -1
        )
        {
            perror("pipe write failed");
        }

        close(pipe_fd[1]);
        close(lock_fd);

        fflush(NULL);

        _exit(
            (
                result == 0 &&
                stats.errors == 0
            )
            ? 0
            : 1
        );
    }

    close(lock_fd);
    close(pipe_fd[1]);

    printf("\n--- Parent Process ---\n");

    printf(
        "Parent waiting for child PID %d...\n",
        pid
    );

    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid failed");

        close(pipe_fd[0]);

        return 1;
    }

    {
        ssize_t bytes_read;

        bytes_read = read(
            pipe_fd[0],
            message,
            sizeof(message) - 1
        );

        if (bytes_read == -1)
        {
            perror("pipe read failed");

            close(pipe_fd[0]);

            return 1;
        }

        if (bytes_read > 0)
        {
            message[bytes_read] = '\0';
        }
    }

    close(pipe_fd[0]);

    printf("\n--- IPC Message ---\n");

    if (message[0] != '\0')
    {
        printf(
            "Parent received: %s\n",
            message
        );
    }
    else
    {
        printf(
            "Parent received no synchronization message\n"
        );
    }

    if (WIFEXITED(status))
    {
        printf(
            "Child exit status: %d\n",
            WEXITSTATUS(status)
        );

        return WEXITSTATUS(status);
    }

    fprintf(
        stderr,
        "Child process did not exit normally\n"
    );

    return 1;
}
