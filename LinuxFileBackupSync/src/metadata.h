#ifndef METADATA_H
#define METADATA_H

typedef enum
{
    FILE_STATE_NEW,
    FILE_STATE_MODIFIED,
    FILE_STATE_UNCHANGED,
    FILE_STATE_ERROR
} FileState;

FileState compare_file_metadata(
    const char *source,
    const char *destination
);

#endif
