#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

volatile sig_atomic_t signal_received = 0;

void handle_signal(int signal_number)
{
    if (signal_number == SIGUSR1)
    {
        signal_received = 1;
    }
}

int main(void)
{
    pid_t pid;
    int status;

    printf("========================================\n");
    printf(" Signal Handling Demonstration\n");
    printf("========================================\n");

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

        /*
         * Register SIGUSR1 handler.
         */
        signal(SIGUSR1, handle_signal);

        printf("Child: Waiting for SIGUSR1...\n");

        /*
         * Wait until a signal is received.
         */
        pause();

        if (signal_received)
        {
            printf("Child: SIGUSR1 received!\n");
            printf("Child: Signal handler executed successfully.\n");
        }

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

        /*
         * Give the child time to install
         * the signal handler.
         */
        sleep(1);

        printf("Parent: Sending SIGUSR1 to child...\n");

        if (kill(pid, SIGUSR1) == -1)
        {
            perror("kill failed");
            return 1;
        }

        printf("Parent: SIGUSR1 sent successfully.\n");

        /*
         * Wait for child to finish.
         */
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            printf("Child exit status: %d\n",
                   WEXITSTATUS(status));
        }

        printf("Parent process completed.\n");
    }

    return 0;
}
