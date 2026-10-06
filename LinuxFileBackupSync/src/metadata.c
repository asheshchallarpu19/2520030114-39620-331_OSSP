#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include "metadata.h"

FileState compare_file_metadata(
    const char *source,
    const char *destination
)
{
    struct stat source_stat;
    struct stat destination_stat;

    if (stat(source, &source_stat) == -1)
    {
        perror("Error reading source metadata");
        return FILE_STATE_ERROR;
    }

    if (stat(destination, &destination_stat) == -1)
    {
        if (errno == ENOENT)
        {
            return FILE_STATE_NEW;
        }

        perror("Error reading destination metadata");
        return FILE_STATE_ERROR;
    }

    if (
        source_stat.st_size != destination_stat.st_size ||
        source_stat.st_mtim.tv_sec != destination_stat.st_mtim.tv_sec ||
        source_stat.st_mtim.tv_nsec != destination_stat.st_mtim.tv_nsec
    )
    {
        return FILE_STATE_MODIFIED;
    }

    return FILE_STATE_UNCHANGED;
}
