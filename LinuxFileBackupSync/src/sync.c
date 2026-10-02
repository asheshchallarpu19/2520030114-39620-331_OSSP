#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sync.h"
#include "file_ops.h"
#include "metadata.h"

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
            fprintf(stderr, "[ERROR] Not a directory: %s\n", path);
            stats->errors++;
            return -1;
        }

        return 0;
    }

    if (errno != ENOENT)
    {
        perror("stat directory failed");
        stats->errors++;
        return -1;
    }

    if (mkdir(path, 0755) == -1)
    {
        perror("mkdir failed");
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

    if (result < 0 || (size_t) result >= output_size)
    {
        return -1;
    }

    return 0;
}

int sync_directory(
    const char *source_dir,
    const char *backup_dir,
    SyncStats *stats
)
{
    DIR *directory;
    struct dirent *entry;

    if (ensure_directory(backup_dir, stats) == -1)
    {
        return -1;
    }

    directory = opendir(source_dir);

    if (directory == NULL)
    {
        perror("opendir failed");
        stats->errors++;
        return -1;
    }

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
            fprintf(stderr, "[ERROR] Path too long\n");
            stats->errors++;
            continue;
        }

        if (lstat(source_path, &file_info) == -1)
        {
            perror("lstat failed");
            stats->errors++;
            continue;
        }

        if (S_ISDIR(file_info.st_mode))
        {
            sync_directory(
                source_path,
                backup_path,
                stats
            );
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
                if (copy_file(source_path, backup_path) == 0)
                {
                    printf("[COPY] %s (NEW)\n", source_path);
                    stats->new_files++;
                }
                else
                {
                    stats->errors++;
                }
            }
            else if (state == FILE_STATE_MODIFIED)
            {
                if (copy_file(source_path, backup_path) == 0)
                {
                    printf("[UPDATE] %s (MODIFIED)\n", source_path);
                    stats->updated_files++;
                }
                else
                {
                    stats->errors++;
                }
            }
            else if (state == FILE_STATE_UNCHANGED)
            {
                printf("[SKIP] %s (UNCHANGED)\n", source_path);
                stats->skipped_files++;
            }
            else
            {
                stats->errors++;
            }
        }
        else
        {
            printf(
                "[SKIP] %s (unsupported file type)\n",
                source_path
            );
        }
    }

    if (closedir(directory) == -1)
    {
        perror("closedir failed");
        stats->errors++;
        return -1;
    }

    return 0;
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
    printf("Errors              : %lu\n", stats->errors);

    printf("========================================\n");
}
