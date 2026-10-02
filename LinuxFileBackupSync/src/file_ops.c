#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#include "file_ops.h"

#define BUFFER_SIZE 4096

int copy_file(const char *source, const char *destination)
{
    int source_fd;
    int destination_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    struct stat source_stat;

    source_fd = open(source, O_RDONLY);

    if (source_fd == -1)
    {
        perror("Error opening source file");
        return -1;
    }

    if (fstat(source_fd, &source_stat) == -1)
    {
        perror("Error reading source metadata");
        close(source_fd);
        return -1;
    }

    destination_fd = open(
        destination,
        O_WRONLY | O_CREAT | O_TRUNC,
        source_stat.st_mode & 0777
    );

    if (destination_fd == -1)
    {
        perror("Error opening destination file");
        close(source_fd);
        return -1;
    }

    while ((bytes_read = read(source_fd, buffer, BUFFER_SIZE)) > 0)
    {
        ssize_t total_written = 0;

        while (total_written < bytes_read)
        {
            ssize_t bytes_written = write(
                destination_fd,
                buffer + total_written,
                bytes_read - total_written
            );

            if (bytes_written == -1)
            {
                if (errno == EINTR)
                    continue;

                perror("Error writing to destination file");

                close(source_fd);
                close(destination_fd);

                return -1;
            }

            total_written += bytes_written;
        }
    }

    if (bytes_read == -1)
    {
        perror("Error reading source file");

        close(source_fd);
        close(destination_fd);

        return -1;
    }

    /* Preserve source modification time */
    struct timespec times[2];

    times[0] = source_stat.st_atim;
    times[1] = source_stat.st_mtim;

    if (futimens(destination_fd, times) == -1)
    {
        perror("Warning: could not preserve file timestamps");
    }

    close(source_fd);
    close(destination_fd);

    return 0;
}
