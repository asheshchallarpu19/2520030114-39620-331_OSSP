#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <time.h>

#include "file_ops.h"

#define BUFFER_SIZE 4096

int copy_file(const char *source, const char *destination)
{
    int source_fd = -1;
    int temp_fd = -1;

    char buffer[BUFFER_SIZE];
    char temp_path[PATH_MAX];

    ssize_t bytes_read;
    struct stat source_stat;

    int result;

    /*
     * Open source file.
     */
    source_fd = open(source, O_RDONLY);

    if (source_fd == -1)
    {
        perror("Error opening source file");
        return -1;
    }

    /*
     * Read source metadata.
     */
    if (fstat(source_fd, &source_stat) == -1)
    {
        perror("Error reading source metadata");
        close(source_fd);
        return -1;
    }

    /*
     * Build a temporary file path in the SAME directory
     * as the final destination.
     *
     * rename() is atomic when source and destination
     * are on the same filesystem.
     */
    result = snprintf(
        temp_path,
        sizeof(temp_path),
        "%s.tmp.XXXXXX",
        destination
    );

    if (result < 0 || (size_t) result >= sizeof(temp_path))
    {
        fprintf(stderr, "Temporary path too long: %s\n", destination);
        close(source_fd);
        return -1;
    }

    /*
     * Create a unique temporary file safely.
     */
    temp_fd = mkstemp(temp_path);

    if (temp_fd == -1)
    {
        perror("Error creating temporary backup file");
        close(source_fd);
        return -1;
    }

    /*
     * Match source permission bits.
     */
    if (fchmod(temp_fd, source_stat.st_mode & 0777) == -1)
    {
        perror("Error setting temporary file permissions");

        close(source_fd);
        close(temp_fd);
        unlink(temp_path);

        return -1;
    }

    /*
     * Copy source data into temporary destination.
     */
    for (;;)
    {
        bytes_read = read(
            source_fd,
            buffer,
            sizeof(buffer)
        );

        if (bytes_read == 0)
        {
            break;
        }

        if (bytes_read == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("Error reading source file");

            close(source_fd);
            close(temp_fd);
            unlink(temp_path);

            return -1;
        }

        ssize_t total_written = 0;

        while (total_written < bytes_read)
        {
            ssize_t bytes_written = write(
                temp_fd,
                buffer + total_written,
                (size_t)(bytes_read - total_written)
            );

            if (bytes_written == -1)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                perror("Error writing temporary backup file");

                close(source_fd);
                close(temp_fd);
                unlink(temp_path);

                return -1;
            }

            total_written += bytes_written;
        }
    }

    /*
     * Preserve source timestamps.
     */
    {
        struct timespec times[2];

        times[0] = source_stat.st_atim;
        times[1] = source_stat.st_mtim;

        if (futimens(temp_fd, times) == -1)
        {
            perror("Error preserving file timestamps");

            close(source_fd);
            close(temp_fd);
            unlink(temp_path);

            return -1;
        }
    }

    /*
     * Flush file data and metadata before replacement.
     */
    if (fsync(temp_fd) == -1)
    {
        perror("Error flushing temporary backup file");

        close(source_fd);
        close(temp_fd);
        unlink(temp_path);

        return -1;
    }

    if (close(source_fd) == -1)
    {
        perror("Error closing source file");

        close(temp_fd);
        unlink(temp_path);

        return -1;
    }

    source_fd = -1;

    if (close(temp_fd) == -1)
    {
        perror("Error closing temporary backup file");

        unlink(temp_path);

        return -1;
    }

    temp_fd = -1;

    /*
     * Atomically replace the destination only after
     * the complete temporary copy succeeded.
     */
    if (rename(temp_path, destination) == -1)
    {
        perror("Error replacing destination file");

        unlink(temp_path);

        return -1;
    }

    return 0;
}
