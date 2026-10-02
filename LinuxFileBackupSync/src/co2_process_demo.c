#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;
    int status;

    printf("========================================\n");
    printf(" CO2 - Process Control Demonstration\n");
    printf("========================================\n");

    printf("Parent PID: %d\n", getpid());

    fflush(NULL);

    pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        return 1;
    }

    if (pid == 0)
    {
        printf("\n--- Child Process ---\n");
        printf("Child PID : %d\n", getpid());
        printf("Parent PID: %d\n", getppid());

        printf("Child: replacing itself using exec...\n");

        fflush(NULL);

        execl(
            "/bin/echo",
            "echo",
            "CO2 exec() executed successfully.",
            (char *) NULL
        );

        /*
         * This executes only if exec() fails.
         */
        perror("exec failed");
        _exit(1);
    }

    printf("\n--- Parent Process ---\n");
    printf("Created child PID: %d\n", pid);
    printf("Parent waiting for child...\n");

    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid failed");
        return 1;
    }

    if (WIFEXITED(status))
    {
        printf(
            "Child exited normally with status: %d\n",
            WEXITSTATUS(status)
        );
    }
    else
    {
        printf("Child did not terminate normally.\n");
        return 1;
    }

    printf("Parent successfully reaped child process.\n");
    printf("Process-control demonstration completed.\n");

    return 0;
}
