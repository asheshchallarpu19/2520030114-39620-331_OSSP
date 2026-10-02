# Linux File Backup and Synchronization System

## Overview

This project is a Linux-based file backup and synchronization system developed in C.

The program recursively scans a source directory and synchronizes files into a backup directory. It detects new, modified, and unchanged files using file metadata and performs only the required backup operations.

The project also demonstrates important Operating Systems and System Programming concepts including process creation, inter-process communication, named pipes, signals, directory traversal, and Linux file system calls.

---

## Main Features

- Recursive directory traversal
- Backup of new files
- Update of modified files
- Skip unchanged files
- Automatic creation of missing backup directories
- File metadata comparison using `stat()`
- Low-level file copying using `open()`, `read()`, `write()`, and `close()`
- File timestamp preservation
- Command-line source and backup directories
- Parent-child process creation using `fork()`
- Anonymous pipe communication
- Named pipe (FIFO) demonstration
- Signal handling using `SIGUSR1`
- Synchronization statistics
- Error handling
- Automated testing
- Valgrind memory testing
- `strace` system-call verification

---

## Project Structure

    LinuxFileBackupSync/
    |
    +-- src/
    |   +-- main.c
    |   +-- file_ops.c
    |   +-- file_ops.h
    |   +-- metadata.c
    |   +-- metadata.h
    |   +-- sync.c
    |   +-- sync.h
    |   +-- fifo_demo.c
    |   +-- signal_demo.c
    |
    +-- tests/
    |   +-- test_backup.sh
    |
    +-- Makefile
    +-- .gitignore
    +-- README.md

---

## Core Modules

### main.c

Responsible for:

- Command-line argument validation
- Source directory validation
- Creating a child process using `fork()`
- Creating an anonymous pipe
- Starting synchronization
- Receiving synchronization status from the child
- Waiting for child completion using `waitpid()`

### sync.c

Responsible for:

- Recursive directory scanning
- Creating backup directories
- Processing regular files
- Calling metadata comparison functions
- Copying new and modified files
- Skipping unchanged files
- Maintaining synchronization statistics

### metadata.c

Compares the source file and backup file using:

- File size
- Modification time

Possible file states are:

- `FILE_STATE_NEW`
- `FILE_STATE_MODIFIED`
- `FILE_STATE_UNCHANGED`
- `FILE_STATE_ERROR`

### file_ops.c

Handles file copying using Linux file operations:

- `open()`
- `read()`
- `write()`
- `close()`
- `fstat()`
- `futimens()`

The implementation also handles partial writes and interrupted system calls.

---

## System Calls and OS Concepts Used

The project demonstrates the use of:

- `fork()`
- `pipe()`
- `waitpid()`
- `open()`
- `read()`
- `write()`
- `close()`
- `stat()`
- `lstat()`
- `fstat()`
- `mkdir()`
- `opendir()`
- `readdir()`
- `closedir()`
- `mkfifo()`
- `unlink()`
- `kill()`
- Signal handling

---

## Build Instructions

The project is designed for Linux or Ubuntu under WSL.

Compile all programs using:

    make

This builds:

    backup_sync
    fifo_demo
    signal_demo

Remove compiled binaries using:

    make clean

---

## Running the Backup Program

Syntax:

    ./backup_sync <source_directory> <backup_directory>

Example:

    ./backup_sync /tmp/source /tmp/backup

The program displays operations such as:

    [COPY] file.txt (NEW)
    [UPDATE] report.txt (MODIFIED)
    [SKIP] notes.txt (UNCHANGED)

After synchronization, a summary is displayed showing:

- Files scanned
- New files copied
- Files updated
- Unchanged files skipped
- Directories created
- Errors

---

## Named Pipe FIFO Demonstration

Run:

    ./fifo_demo

The program creates the FIFO at:

    /tmp/ossp_backup_fifo

The child process sends:

    BACKUP_COMPLETED

to the parent process through the named pipe.

`/tmp` is used because Unix FIFOs may not be supported inside Windows-mounted WSL paths such as `/mnt/c`.

---

## Signal Handling Demonstration

Run:

    ./signal_demo

The parent process sends `SIGUSR1` to the child process.

The child waits for the signal and executes its registered signal handler after receiving it.

---

## Automated Testing

Run:

    ./tests/test_backup.sh

The automated test suite verifies:

1. Initial file backup
2. Recursive directory copying
3. Detection of unchanged files
4. Detection and update of modified files
5. Deep recursive directory synchronization
6. Invalid source directory rejection

Successful execution ends with:

    ALL AUTOMATED TESTS PASSED

---

## Memory Testing

The project was tested using Valgrind:

    valgrind --leak-check=full --show-leak-kinds=all \
    ./backup_sync /tmp/source /tmp/backup

Testing produced:

    All heap blocks were freed -- no leaks are possible
    ERROR SUMMARY: 0 errors

---

## System Call Verification

Linux system calls were verified using `strace`.

Example:

    strace -f -yy \
    -e trace=openat,read,write,close,newfstatat,mkdir \
    ./backup_sync /tmp/source /tmp/backup

The trace confirms actual Linux system calls for:

- File metadata access
- File opening
- Reading
- Writing
- Closing
- Directory access
- Pipe communication

---

## Synchronization Workflow

    Source Directory
           |
           v
    Recursive Directory Scan
           |
           v
    Read File Metadata
           |
           v
    Compare Source and Backup
           |
        +--+--+
        |     |
       NEW  MODIFIED
        |     |
       COPY  UPDATE
        |
    UNCHANGED
        |
       SKIP
           |
           v
    Synchronization Summary
           |
           v
    Status sent to Parent through Pipe

---

## Current Synchronization Behavior

The program currently performs one-way synchronization:

    Source Directory -> Backup Directory

New source files are copied.

Modified source files are updated.

Unchanged files are skipped.

Files deleted from the source directory are currently retained in the backup directory.

Symbolic links and unsupported special file types are skipped.

---

## Testing Completed

The project has successfully completed:

- Compilation with GCC
- Recursive backup testing
- New file detection testing
- Modified file detection testing
- Unchanged file testing
- Deep directory testing
- Invalid source testing
- FIFO IPC testing
- Signal handling testing
- Valgrind memory testing
- `strace` system-call verification
- Automated test suite

---

## Future Enhancements

Possible future enhancements include:

- POSIX thread-based parallel synchronization
- Mutex-protected shared statistics
- Bidirectional synchronization
- File deletion synchronization
- Backup versioning
- Checksum-based change detection
- Scheduled automatic backups
- Log files
- Configuration file support
- Graphical user interface

---

## Development Environment

- Programming Language: C
- Operating System: Linux / Ubuntu / WSL
- Compiler: GCC
- Build Tool: Make
- Memory Testing: Valgrind
- System Call Analysis: strace
- Automated Testing: Bash

---

## Project Status

The core Linux File Backup and Synchronization System is implemented and tested successfully.

---

## Course Outcome Mapping

### CO4 - Virtual Memory and Process Address Space

Implemented through `src/memory_demo.c`.

The demonstration includes:

- Virtual memory allocation using `mmap()`
- System page-size detection using `sysconf(_SC_PAGESIZE)`
- Process creation using `fork()`
- Parent and child process address spaces
- Copy-on-Write behavior
- Memory modification after `fork()`
- Memory cleanup using `munmap()`

Run:

    ./memory_demo

The child changes the mapped value from 100 to 200, while the parent still sees 100, demonstrating Copy-on-Write behavior.

### CO5 - File Systems and Linux File I/O

Implemented mainly through the backup and synchronization system.

The project uses:

- File descriptors
- `open()`
- `read()`
- `write()`
- `close()`
- `stat()`
- `lstat()`
- `fstat()`
- `futimens()`
- `mkdir()`
- `opendir()`
- `readdir()`
- `closedir()`
- Recursive directory traversal
- File metadata comparison
- File and directory synchronization

Run:

    ./backup_sync <source_directory> <backup_directory>

### CO6 - Concurrency and Synchronization

Implemented through `src/thread_demo.c`.

The demonstration includes:

- POSIX threads using `pthread_create()`
- Thread completion using `pthread_join()`
- Shared data between threads
- Race-condition prevention
- Mutex locking using `pthread_mutex_lock()`
- Mutex unlocking using `pthread_mutex_unlock()`
- Mutex initialization and destruction
- Multiple worker threads updating shared statistics safely

Run:

    ./thread_demo

Four threads each process five simulated files. The shared counter is protected by a mutex and finishes with the expected total of 20.

### Additional Operating-System Demonstrations

The project also includes:

- Anonymous pipe IPC using `pipe()`
- Named pipe IPC using `mkfifo()`
- Signal handling using `SIGUSR1`
- Process synchronization using `waitpid()`
- Memory validation using Valgrind
- System-call verification using `strace`

