#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    long page_size;
    int *shared_value;
    pid_t pid;
    int status;

    printf("========================================\n");
    printf(" Virtual Memory and COW Demonstration\n");
    printf("========================================\n");

    page_size = sysconf(_SC_PAGESIZE);

    if (page_size == -1)
    {
        perror("sysconf failed");
        return 1;
    }

    printf("System page size: %ld bytes\n", page_size);

    /*
     * Create one private anonymous memory mapping.
     *
     * MAP_PRIVATE causes changes made after fork()
     * to use Copy-on-Write semantics.
     */
    shared_value = mmap(
        NULL,
        (size_t) page_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (shared_value == MAP_FAILED)
    {
        perror("mmap failed");
        return 1;
    }

    *shared_value = 100;

    printf(
        "Parent PID: %d\n",
        getpid()
    );

    printf(
        "Mapped virtual address: %p\n",
        (void *) shared_value
    );

    printf(
        "Parent value before fork: %d\n",
        *shared_value
    );

    fflush(NULL);

    pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        munmap(shared_value, (size_t) page_size);
        return 1;
    }

    if (pid == 0)
    {
        printf("\n--- Child Process ---\n");
        printf("Child PID: %d\n", getpid());

        printf(
            "Child inherited address: %p\n",
            (void *) shared_value
        );

        printf(
            "Child value before modification: %d\n",
            *shared_value
        );

        /*
         * This write causes Copy-on-Write.
         * The child's page becomes private.
         */
        *shared_value = 200;

        printf(
            "Child value after modification : %d\n",
            *shared_value
        );

        if (munmap(
                shared_value,
                (size_t) page_size
            ) == -1)
        {
            perror("child munmap failed");
            _exit(1);
        }

        fflush(NULL);
        _exit(0);
    }

    printf("\n--- Parent Process ---\n");

    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid failed");
        munmap(shared_value, (size_t) page_size);
        return 1;
    }

    printf(
        "Parent value after child modified memory: %d\n",
        *shared_value
    );

    printf(
        "Parent value remains 100 because of Copy-on-Write.\n"
    );

    if (munmap(
            shared_value,
            (size_t) page_size
        ) == -1)
    {
        perror("parent munmap failed");
        return 1;
    }

    if (WIFEXITED(status))
    {
        printf(
            "Child exit status: %d\n",
            WEXITSTATUS(status)
        );
    }

    printf("Virtual memory demonstration completed.\n");

    return 0;
}
