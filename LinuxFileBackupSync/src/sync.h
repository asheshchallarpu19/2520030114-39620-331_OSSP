#ifndef SYNC_H
#define SYNC_H

typedef struct
{
    unsigned long scanned_files;
    unsigned long new_files;
    unsigned long updated_files;
    unsigned long skipped_files;
    unsigned long directories_created;
    unsigned long errors;

} SyncStats;

int sync_directory(
    const char *source_dir,
    const char *backup_dir,
    SyncStats *stats
);

void print_sync_summary(
    const SyncStats *stats
);

#endif
