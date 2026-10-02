#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "sync.h"

static int validate_source_directory(const char *path)
{
    struct stat st;

    if (stat(path, &st) == -1)
    {
        perror("Source directory error");
        return -1;
    }

    if (!S_ISDIR(st.st_mode))
    {
        fprintf(stderr, "Source is not a directory: %s\n", path);
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;
    int pipe_fd[2];
    char message[64] = {0};

    if (argc != 3)
    {
        fprintf(
            stderr,
            "Usage: %s <source_directory> <backup_directory>\n",
            argv[0]
        );

        return 1;
    }

    if (validate_source_directory(argv[1]) == -1)
    {
        return 1;
    }

    printf("========================================\n");
    printf(" Linux File Backup & Synchronization\n");
    printf("========================================\n");

    printf("Source : %s\n", argv[1]);
    printf("Backup : %s\n", argv[2]);
    printf("Main Process PID: %d\n", getpid());

    fflush(NULL);

    if (pipe(pipe_fd) == -1)
    {
        perror("pipe failed");
        return 1;
    }

    pid = fork();

    if (pid < 0)
    {
        perror("fork failed");

        close(pipe_fd[0]);
        close(pipe_fd[1]);

        return 1;
    }

    if (pid == 0)
    {
        SyncStats stats = {0};
        int result;

        close(pipe_fd[0]);

        printf(
            "\n--- Child Process: Synchronization Worker ---\n"
        );

        result = sync_directory(
            argv[1],
            argv[2],
            &stats
        );

        print_sync_summary(&stats);

        if (result == 0 && stats.errors == 0)
        {
            strcpy(message, "SYNC_SUCCESS");
        }
        else
        {
            strcpy(
                message,
                "SYNC_COMPLETED_WITH_ERRORS"
            );
        }

        write(
            pipe_fd[1],
            message,
            strlen(message) + 1
        );

        close(pipe_fd[1]);

        fflush(NULL);

        _exit(
            (result == 0 && stats.errors == 0)
            ? 0
            : 1
        );
    }

    close(pipe_fd[1]);

    printf("\n--- Parent Process ---\n");
    printf(
        "Parent waiting for child PID %d...\n",
        pid
    );

    waitpid(pid, &status, 0);

    read(
        pipe_fd[0],
        message,
        sizeof(message)
    );

    close(pipe_fd[0]);

    printf("\n--- IPC Message ---\n");
    printf("Parent received: %s\n", message);

    if (WIFEXITED(status))
    {
        printf(
            "Child exit status: %d\n",
            WEXITSTATUS(status)
        );

        return WEXITSTATUS(status);
    }

    return 1;
}
