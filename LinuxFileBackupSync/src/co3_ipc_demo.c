#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MESSAGE_SIZE 128

int main(void)
{
    int sockets[2];
    pid_t pid;
    int status;

    char *shared_memory;

    printf("========================================\n");
    printf(" CO3 - IPC Demonstration\n");
    printf(" Unix Socket + Shared Memory\n");
    printf("========================================\n");

    /*
     * Create a Unix-domain socket pair.
     */
    if (socketpair(
            AF_UNIX,
            SOCK_STREAM,
            0,
            sockets
        ) == -1)
    {
        perror("socketpair failed");
        return 1;
    }

    /*
     * Create shared memory visible to parent and child.
     */
    shared_memory = mmap(
        NULL,
        MESSAGE_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if (shared_memory == MAP_FAILED)
    {
        perror("mmap failed");

        close(sockets[0]);
        close(sockets[1]);

        return 1;
    }

    strcpy(
        shared_memory,
        "Shared memory initialized by parent"
    );

    fflush(NULL);

    pid = fork();

    if (pid < 0)
    {
        perror("fork failed");

        close(sockets[0]);
        close(sockets[1]);

        munmap(shared_memory, MESSAGE_SIZE);

        return 1;
    }

    if (pid == 0)
    {
        char socket_message[] =
            "Hello parent from Unix-domain socket";

        close(sockets[0]);

        printf("\n--- Child Process ---\n");

        printf(
            "Child reads shared memory: %s\n",
            shared_memory
        );

        strcpy(
            shared_memory,
            "Shared memory updated by child"
        );

        if (write(
                sockets[1],
                socket_message,
                strlen(socket_message) + 1
            ) == -1)
        {
            perror("socket write failed");

            close(sockets[1]);
            munmap(shared_memory, MESSAGE_SIZE);

            _exit(1);
        }

        printf(
            "Child sent socket message successfully.\n"
        );

        close(sockets[1]);

        munmap(
            shared_memory,
            MESSAGE_SIZE
        );

        fflush(NULL);
        _exit(0);
    }

    close(sockets[1]);

    printf("\n--- Parent Process ---\n");

    {
        char buffer[MESSAGE_SIZE] = {0};

        ssize_t bytes_read = read(
            sockets[0],
            buffer,
            sizeof(buffer) - 1
        );

        if (bytes_read == -1)
        {
            perror("socket read failed");

            close(sockets[0]);
            munmap(shared_memory, MESSAGE_SIZE);

            return 1;
        }

        printf(
            "Parent received through Unix socket: %s\n",
            buffer
        );
    }

    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid failed");

        close(sockets[0]);
        munmap(shared_memory, MESSAGE_SIZE);

        return 1;
    }

    printf(
        "Parent sees shared memory: %s\n",
        shared_memory
    );

    close(sockets[0]);

    if (munmap(
            shared_memory,
            MESSAGE_SIZE
        ) == -1)
    {
        perror("munmap failed");
        return 1;
    }

    if (WIFEXITED(status))
    {
        printf(
            "Child exit status: %d\n",
            WEXITSTATUS(status)
        );
    }

    printf("\nCO3 IPC demonstration completed.\n");

    return 0;
}
