#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include "file_ops.h"

int main(void)
{
    const char *source = "data/source/sample.txt";
    const char *destination = "data/backup/sample.txt";

    pid_t pid;
    int status;

    int pipe_fd[2];
    char message[100];

    printf("========================================\n");
    printf(" Linux File Backup & Synchronization\n");
    printf("========================================\n");

    printf("Main Process PID: %d\n", getpid());

    /*
     * Create anonymous pipe
     *
     * pipe_fd[0] = read end
     * pipe_fd[1] = write end
     */
    if (pipe(pipe_fd) == -1)
    {
        perror("pipe failed");
        return 1;
    }

    printf("\nAnonymous pipe created successfully.\n");

    pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        return 1;
    }

    if (pid == 0)
    {
        /* =========================
           CHILD PROCESS
           ========================= */

        close(pipe_fd[0]);

        printf("\n--- Child Process ---\n");
        printf("Child PID : %d\n", getpid());
        printf("Parent PID: %d\n", getppid());

        printf("Child is performing backup...\n");

        if (copy_file(source, destination) == 0)
        {
            printf("Child: Backup completed successfully!\n");

            strcpy(message, "BACKUP_SUCCESS");

            write(pipe_fd[1], message, strlen(message) + 1);

            close(pipe_fd[1]);

            exit(0);
        }
        else
        {
            printf("Child: Backup failed!\n");

            strcpy(message, "BACKUP_FAILED");

            write(pipe_fd[1], message, strlen(message) + 1);

            close(pipe_fd[1]);

            exit(1);
        }
    }
    else
    {
        /* =========================
           PARENT PROCESS
           ========================= */

        close(pipe_fd[1]);

        printf("\n--- Parent Process ---\n");
        printf("Parent PID: %d\n", getpid());
        printf("Child PID : %d\n", pid);

        printf("Parent is waiting for child...\n");

        waitpid(pid, &status, 0);

        /*
         * Read message sent by child
         */
        read(pipe_fd[0], message, sizeof(message));

        close(pipe_fd[0]);

        printf("\n--- IPC Message ---\n");
        printf("Parent received from child: %s\n", message);

        if (WIFEXITED(status))
        {
            printf("Child exit status: %d\n",
                   WEXITSTATUS(status));
        }

        printf("\nParent process completed.\n");
    }

    return 0;
}
