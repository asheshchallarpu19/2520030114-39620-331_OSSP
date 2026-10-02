#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define FIFO_DIR "/tmp"
#define FIFO_PATH "/tmp/ossp_backup_fifo"

static int ensure_directory(const char *path)
{
    if (mkdir(path, 0755) == -1 && errno != EEXIST)
    {
        perror("mkdir failed");
        return -1;
    }

    return 0;
}

int main(void)
{
    pid_t pid;
    int fifo_fd;
    int status;

    const char message[] = "BACKUP_COMPLETED";
    char buffer[100] = {0};

    printf("========================================\n");
    printf(" Named Pipe (FIFO) Demonstration\n");
    printf("========================================\n");

    /*
     * Ensure the FIFO directory exists.
     */
    if (ensure_directory("data") == -1)
    {
        return 1;
    }

    if (ensure_directory(FIFO_DIR) == -1)
    {
        return 1;
    }

    /*
     * Remove any stale FIFO from an earlier run.
     */
    if (unlink(FIFO_PATH) == -1 && errno != ENOENT)
    {
        perror("unlink old FIFO failed");
        return 1;
    }

    /*
     * Create the named pipe.
     */
    if (mkfifo(FIFO_PATH, 0666) == -1)
    {
        perror("mkfifo failed");
        return 1;
    }

    printf("FIFO created: %s\n", FIFO_PATH);

    fflush(NULL);

    pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        unlink(FIFO_PATH);
        return 1;
    }

    if (pid == 0)
    {
        ssize_t bytes_written;

        printf("\n--- Child Process ---\n");
        printf("Child PID: %d\n", getpid());

        fifo_fd = open(FIFO_PATH, O_WRONLY);

        if (fifo_fd == -1)
        {
            perror("open FIFO for writing failed");
            exit(1);
        }

        printf("Child: Sending message through FIFO...\n");

        bytes_written = write(
            fifo_fd,
            message,
            strlen(message) + 1
        );

        if (bytes_written == -1)
        {
            perror("FIFO write failed");
            close(fifo_fd);
            exit(1);
        }

        close(fifo_fd);

        printf("Child: Message sent successfully.\n");

        exit(0);
    }
    else
    {
        ssize_t bytes_read;

        printf("\n--- Parent Process ---\n");
        printf("Parent PID: %d\n", getpid());
        printf("Child PID : %d\n", pid);

        fifo_fd = open(FIFO_PATH, O_RDONLY);

        if (fifo_fd == -1)
        {
            perror("open FIFO for reading failed");
            unlink(FIFO_PATH);
            return 1;
        }

        printf("Parent: Waiting for message...\n");

        bytes_read = read(
            fifo_fd,
            buffer,
            sizeof(buffer) - 1
        );

        if (bytes_read == -1)
        {
            perror("FIFO read failed");

            close(fifo_fd);
            unlink(FIFO_PATH);

            return 1;
        }

        buffer[bytes_read] = '\0';

        close(fifo_fd);

        printf("Parent received: %s\n", buffer);

        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            printf(
                "Child exit status: %d\n",
                WEXITSTATUS(status)
            );
        }

        if (unlink(FIFO_PATH) == -1)
        {
            perror("unlink FIFO failed");
            return 1;
        }

        printf("FIFO removed.\n");
        printf("Parent process completed.\n");
    }

    return 0;
}
