#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sync.h"
#include "metadata.h"
#include "worker_pool.h"

static int ensure_directory(
    const char *path,
    SyncStats *stats
)
{
    struct stat st;

    if (stat(path, &st) == 0)
    {
        if (!S_ISDIR(st.st_mode))
        {
            fprintf(
                stderr,
                "[ERROR] Destination exists but is not a directory: %s\n",
                path
            );

            stats->errors++;
            return -1;
        }

        return 0;
    }

    if (errno != ENOENT)
    {
        fprintf(
            stderr,
            "[ERROR] Cannot access directory %s: %s\n",
            path,
            strerror(errno)
        );

        stats->errors++;
        return -1;
    }

    if (mkdir(path, 0755) == -1)
    {
        fprintf(
            stderr,
            "[ERROR] Cannot create directory %s: %s\n",
            path,
            strerror(errno)
        );

        stats->errors++;
        return -1;
    }

    printf("[MKDIR] %s\n", path);
    stats->directories_created++;

    return 0;
}

static int join_path(
    char *output,
    size_t output_size,
    const char *directory,
    const char *name
)
{
    int result;

    result = snprintf(
        output,
        output_size,
        "%s/%s",
        directory,
        name
    );

    if (
        result < 0 ||
        (size_t) result >= output_size
    )
    {
        return -1;
    }

    return 0;
}


static int remove_backup_tree(
    const char *path,
    SyncStats *stats
)
{
    struct stat st;

    if (lstat(path, &st) == -1)
    {
        fprintf(
            stderr,
            "[ERROR] Cannot inspect backup entry %s: %s\n",
            path,
            strerror(errno)
        );

        stats->errors++;
        return -1;
    }

    if (S_ISDIR(st.st_mode))
    {
        DIR *directory;
        struct dirent *entry;
        int had_error = 0;

        directory = opendir(path);

        if (directory == NULL)
        {
            fprintf(
                stderr,
                "[ERROR] Cannot open backup directory %s: %s\n",
                path,
                strerror(errno)
            );

            stats->errors++;
            return -1;
        }

        errno = 0;

        while ((entry = readdir(directory)) != NULL)
        {
            char child_path[PATH_MAX];

            if (
                strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0
            )
            {
                errno = 0;
                continue;
            }

            if (
                join_path(
                    child_path,
                    sizeof(child_path),
                    path,
                    entry->d_name
                ) == -1
            )
            {
                fprintf(
                    stderr,
                    "[ERROR] Backup path too long: %s/%s\n",
                    path,
                    entry->d_name
                );

                stats->errors++;
                had_error = 1;
                errno = 0;
                continue;
            }

            if (
                remove_backup_tree(
                    child_path,
                    stats
                ) == -1
            )
            {
                had_error = 1;
            }

            errno = 0;
        }

        if (errno != 0)
        {
            fprintf(
                stderr,
                "[ERROR] Failed while reading backup directory %s: %s\n",
                path,
                strerror(errno)
            );

            stats->errors++;
            had_error = 1;
        }

        if (closedir(directory) == -1)
        {
            fprintf(
                stderr,
                "[ERROR] Cannot close backup directory %s: %s\n",
                path,
                strerror(errno)
            );

            stats->errors++;
            had_error = 1;
        }

        if (had_error)
        {
            return -1;
        }

        if (rmdir(path) == -1)
        {
            fprintf(
                stderr,
                "[ERROR] Cannot delete backup directory %s: %s\n",
                path,
                strerror(errno)
            );

            stats->errors++;
            return -1;
        }

        printf(
            "[DELETE-DIR] %s\n",
            path
        );

        stats->deleted_directories++;

        return 0;
    }

    if (unlink(path) == -1)
    {
        fprintf(
            stderr,
            "[ERROR] Cannot delete backup file %s: %s\n",
            path,
            strerror(errno)
        );

        stats->errors++;
        return -1;
    }

    printf(
        "[DELETE] %s\n",
        path
    );

    stats->deleted_files++;

    return 0;
}

int delete_extraneous_entries(
    const char *source_dir,
    const char *backup_dir,
    SyncStats *stats
)
{
    DIR *directory;
    struct dirent *entry;
    int had_error = 0;

    if (
        source_dir == NULL ||
        backup_dir == NULL ||
        stats == NULL
    )
    {
        return -1;
    }

    directory = opendir(backup_dir);

    if (directory == NULL)
    {
        fprintf(
            stderr,
            "[ERROR] Cannot scan backup directory %s: %s\n",
            backup_dir,
            strerror(errno)
        );

        stats->errors++;
        return -1;
    }

    errno = 0;

    while ((entry = readdir(directory)) != NULL)
    {
        char source_path[PATH_MAX];
        char backup_path[PATH_MAX];

        struct stat source_info;
        struct stat backup_info;

        if (
            strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, ".backup_sync.lock") == 0
        )
        {
            errno = 0;
            continue;
        }

        if (
            join_path(
                source_path,
                sizeof(source_path),
                source_dir,
                entry->d_name
            ) == -1 ||

            join_path(
                backup_path,
                sizeof(backup_path),
                backup_dir,
                entry->d_name
            ) == -1
        )
        {
            fprintf(
                stderr,
                "[ERROR] Path too long while checking deletion\n"
            );

            stats->errors++;
            had_error = 1;
            errno = 0;
            continue;
        }

        if (lstat(backup_path, &backup_info) == -1)
        {
            fprintf(
                stderr,
                "[ERROR] Cannot inspect backup entry %s: %s\n",
                backup_path,
                strerror(errno)
            );

            stats->errors++;
            had_error = 1;
            errno = 0;
            continue;
        }

        if (lstat(source_path, &source_info) == -1)
        {
            if (errno == ENOENT)
            {
                if (
                    remove_backup_tree(
                        backup_path,
                        stats
                    ) == -1
                )
                {
                    had_error = 1;
                }

                errno = 0;
                continue;
            }

            fprintf(
                stderr,
                "[ERROR] Cannot inspect source entry %s: %s\n",
                source_path,
                strerror(errno)
            );

            stats->errors++;
            had_error = 1;
            errno = 0;
            continue;
        }

        /*
         * Handle type changes safely.
         *
         * Example:
         * source used to contain a directory called data,
         * but now contains a regular file called data.
         */
        if (
            S_ISDIR(backup_info.st_mode) !=
            S_ISDIR(source_info.st_mode)
        )
        {
            if (
                remove_backup_tree(
                    backup_path,
                    stats
                ) == -1
            )
            {
                had_error = 1;
            }

            errno = 0;
            continue;
        }

        /*
         * If both entries are directories, recurse.
         */
        if (
            S_ISDIR(backup_info.st_mode) &&
            S_ISDIR(source_info.st_mode)
        )
        {
            if (
                delete_extraneous_entries(
                    source_path,
                    backup_path,
                    stats
                ) == -1
            )
            {
                had_error = 1;
            }
        }

        errno = 0;
    }

    if (errno != 0)
    {
        fprintf(
            stderr,
            "[ERROR] Failed while scanning backup directory %s: %s\n",
            backup_dir,
            strerror(errno)
        );

        stats->errors++;
        had_error = 1;
    }

    if (closedir(directory) == -1)
    {
        fprintf(
            stderr,
            "[ERROR] Cannot close backup directory %s: %s\n",
            backup_dir,
            strerror(errno)
        );

        stats->errors++;
        had_error = 1;
    }

    return had_error ? -1 : 0;
}

int sync_directory(
    const char *source_dir,
    const char *backup_dir,
    SyncStats *stats,
    struct WorkerPool *pool
)
{
    DIR *directory;
    struct dirent *entry;

    int had_error = 0;

    if (
        stats == NULL ||
        pool == NULL
    )
    {
        fprintf(
            stderr,
            "[ERROR] Invalid synchronization context\n"
        );

        return -1;
    }

    if (ensure_directory(backup_dir, stats) == -1)
    {
        return -1;
    }

    directory = opendir(source_dir);

    if (directory == NULL)
    {
        fprintf(
            stderr,
            "[ERROR] Cannot open source directory %s: %s\n",
            source_dir,
            strerror(errno)
        );

        stats->errors++;
        return -1;
    }

    errno = 0;

    while ((entry = readdir(directory)) != NULL)
    {
        char source_path[PATH_MAX];
        char backup_path[PATH_MAX];
        struct stat file_info;

        if (
            strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0
        )
        {
            errno = 0;
            continue;
        }

        if (
            join_path(
                source_path,
                sizeof(source_path),
                source_dir,
                entry->d_name
            ) == -1
        )
        {
            fprintf(
                stderr,
                "[ERROR] Source path too long: %s/%s\n",
                source_dir,
                entry->d_name
            );

            stats->errors++;
            had_error = 1;
            errno = 0;
            continue;
        }

        if (
            join_path(
                backup_path,
                sizeof(backup_path),
                backup_dir,
                entry->d_name
            ) == -1
        )
        {
            fprintf(
                stderr,
                "[ERROR] Backup path too long: %s/%s\n",
                backup_dir,
                entry->d_name
            );

            stats->errors++;
            had_error = 1;
            errno = 0;
            continue;
        }

        if (lstat(source_path, &file_info) == -1)
        {
            fprintf(
                stderr,
                "[ERROR] Cannot inspect %s: %s\n",
                source_path,
                strerror(errno)
            );

            stats->errors++;
            had_error = 1;
            errno = 0;
            continue;
        }

        if (S_ISDIR(file_info.st_mode))
        {
            if (
                sync_directory(
                    source_path,
                    backup_path,
                    stats,
                    pool
                ) == -1
            )
            {
                had_error = 1;
            }
        }
        else if (S_ISREG(file_info.st_mode))
        {
            FileState state;

            stats->scanned_files++;

            state = compare_file_metadata(
                source_path,
                backup_path
            );

            if (state == FILE_STATE_NEW)
            {
                /*
                 * Submit the real file copy to a worker thread.
                 */
                if (
                    worker_pool_submit(
                        pool,
                        source_path,
                        backup_path,
                        COPY_JOB_NEW
                    ) == -1
                )
                {
                    fprintf(
                        stderr,
                        "[ERROR] Failed to queue new file: %s\n",
                        source_path
                    );

                    stats->errors++;
                    had_error = 1;
                }
            }
            else if (state == FILE_STATE_MODIFIED)
            {
                /*
                 * Submit the update to a worker thread.
                 */
                if (
                    worker_pool_submit(
                        pool,
                        source_path,
                        backup_path,
                        COPY_JOB_MODIFIED
                    ) == -1
                )
                {
                    fprintf(
                        stderr,
                        "[ERROR] Failed to queue modified file: %s\n",
                        source_path
                    );

                    stats->errors++;
                    had_error = 1;
                }
            }
            else if (state == FILE_STATE_UNCHANGED)
            {
                printf(
                    "[SKIP] %s (UNCHANGED)\n",
                    source_path
                );

                stats->skipped_files++;
            }
            else
            {
                fprintf(
                    stderr,
                    "[ERROR] Metadata comparison failed: %s\n",
                    source_path
                );

                stats->errors++;
                had_error = 1;
            }
        }
        else
        {
            printf(
                "[SKIP] %s (unsupported file type)\n",
                source_path
            );
        }

        errno = 0;
    }

    if (errno != 0)
    {
        fprintf(
            stderr,
            "[ERROR] Failed while reading directory %s: %s\n",
            source_dir,
            strerror(errno)
        );

        stats->errors++;
        had_error = 1;
    }

    if (closedir(directory) == -1)
    {
        fprintf(
            stderr,
            "[ERROR] Cannot close directory %s: %s\n",
            source_dir,
            strerror(errno)
        );

        stats->errors++;
        had_error = 1;
    }

    return had_error ? -1 : 0;
}

void print_sync_summary(
    const SyncStats *stats
)
{
    printf("\n========================================\n");
    printf(" Synchronization Summary\n");
    printf("========================================\n");

    printf("Files scanned       : %lu\n", stats->scanned_files);
    printf("New files copied    : %lu\n", stats->new_files);
    printf("Files updated       : %lu\n", stats->updated_files);
    printf("Unchanged skipped   : %lu\n", stats->skipped_files);
    printf("Directories created : %lu\n", stats->directories_created);
    printf("Files deleted       : %lu\n", stats->deleted_files);
    printf("Directories deleted : %lu\n", stats->deleted_directories);
    printf("Bytes copied        : %llu\n", stats->bytes_copied);
    printf("Worker threads      : %lu\n", stats->worker_threads);
    printf("Elapsed time        : %.3f seconds\n", stats->elapsed_seconds);
    printf("Errors              : %lu\n", stats->errors);

    printf("========================================\n");
}
