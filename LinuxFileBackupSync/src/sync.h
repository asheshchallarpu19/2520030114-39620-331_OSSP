#ifndef SYNC_H
#define SYNC_H

struct WorkerPool;

typedef struct
{
    unsigned long scanned_files;
    unsigned long new_files;
    unsigned long updated_files;
    unsigned long skipped_files;
    unsigned long directories_created;

    unsigned long deleted_files;
    unsigned long deleted_directories;

    unsigned long long bytes_copied;
    unsigned long worker_threads;
    double elapsed_seconds;

    unsigned long errors;

} SyncStats;

int sync_directory(
    const char *source_dir,
    const char *backup_dir,
    SyncStats *stats,
    struct WorkerPool *pool
);

/*
 * Remove backup entries that no longer exist in the source.
 *
 * This function is ONLY called when --delete is enabled.
 */
int delete_extraneous_entries(
    const char *source_dir,
    const char *backup_dir,
    SyncStats *stats
);

void print_sync_summary(
    const SyncStats *stats
);

#endif
