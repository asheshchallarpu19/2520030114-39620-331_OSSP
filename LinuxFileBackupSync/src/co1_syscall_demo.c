#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>

int main(void)
{
    pid_t libc_pid;
    long syscall_pid;

    const char message[] =
        "Message written using the Linux write system call.\n";

    printf("========================================\n");
    printf(" CO1 - OS Service Layer Demonstration\n");
    printf("========================================\n");

    /*
     * getpid() is a normal user-space library interface.
     */
    libc_pid = getpid();

    /*
     * syscall() directly requests a service from the Linux kernel.
     */
    syscall_pid = syscall(SYS_getpid);

    printf("PID using getpid()        : %d\n", libc_pid);
    printf("PID using direct syscall(): %ld\n", syscall_pid);

    if ((long) libc_pid == syscall_pid)
    {
        printf("Result: Both methods identify the same process.\n");
    }
    else
    {
        printf("Result: PID mismatch detected.\n");
        return 1;
    }

    printf("\nUser-space program requesting kernel service...\n");

    if (syscall(
            SYS_write,
            STDOUT_FILENO,
            message,
            strlen(message)
        ) == -1)
    {
        perror("SYS_write failed");
        return 1;
    }

    printf("\nConcept demonstrated:\n");
    printf("User Program -> System Call Interface -> Linux Kernel -> Hardware/Resource\n");

    printf("\nThis program runs in user space.\n");
    printf("The kernel performs privileged operations requested through system calls.\n");

    return 0;
}
