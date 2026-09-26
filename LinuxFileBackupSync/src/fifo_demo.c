#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define FIFO_PATH "data/sync/backup_fifo"

int main(void)
{
    pid_t pid;
    int fifo_fd;
    int status;

    char message[] = "BACKUP_COMPLETED";
    char buffer[100];

    printf("========================================\n");
    printf(" Named Pipe (FIFO) Demonstration\n");
    printf("========================================\n");

    /*
     * Create the named pipe.
     */
    if (mkfifo(FIFO_PATH, 0666) == -1)
    {
        /*
         * FIFO may already exist from an earlier run.
         * We continue in that case.
         */
        printf("FIFO may already exist. Continuing...\n");
    }
    else
    {
        printf("FIFO created: %s\n", FIFO_PATH);
    }

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

        printf("\n--- Child Process ---\n");
        printf("Child PID: %d\n", getpid());

        fifo_fd = open(FIFO_PATH, O_WRONLY);

        if (fifo_fd == -1)
        {
            perror("open FIFO for writing failed");
            exit(1);
        }

        printf("Child: Sending message through FIFO...\n");

        write(fifo_fd, message, strlen(message) + 1);

        close(fifo_fd);

        printf("Child: Message sent successfully.\n");

        exit(0);
    }
    else
    {
        /* =========================
           PARENT PROCESS
           ========================= */

        printf("\n--- Parent Process ---\n");
        printf("Parent PID: %d\n", getpid());
        printf("Child PID : %d\n", pid);

        fifo_fd = open(FIFO_PATH, O_RDONLY);

        if (fifo_fd == -1)
        {
            perror("open FIFO for reading failed");
            return 1;
        }

        printf("Parent: Waiting for message...\n");

        read(fifo_fd, buffer, sizeof(buffer));

        close(fifo_fd);

        printf("Parent received: %s\n", buffer);

        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            printf("Child exit status: %d\n",
                   WEXITSTATUS(status));
        }

        /*
         * Remove the FIFO after use.
         */
        unlink(FIFO_PATH);

        printf("FIFO removed.\n");
        printf("Parent process completed.\n");
    }

    return 0;
}
