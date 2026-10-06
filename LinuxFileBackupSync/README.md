Yes — here is the **entire final README in one giant block** so you can copy everything at once into Nano.

```markdown
# Linux File Backup and Synchronization System

## Overview

The **Linux File Backup and Synchronization System** is a C-based Operating Systems and System Programming project developed for Linux / Ubuntu / WSL.

The application performs one-way synchronization:

```text
Source Directory -> Backup Directory
```

It recursively scans a source directory, detects new and modified files, skips unchanged files, and synchronizes data into a backup directory.

The final implementation includes real POSIX multithreading, safe atomic file copying, exclusive backup locking, optional mirror deletion, parent-child process control, IPC, runtime statistics, automated testing, memory analysis, concurrency analysis, and system-call verification.

---

## Main Features

- Recursive directory traversal
- Backup of new files
- Update of modified files
- Skip unchanged files
- Automatic creation of directories
- Nanosecond-resolution modification-time comparison
- Real multithreaded file-copy worker pool
- Configurable worker count using `--threads N`
- Safe temporary-file copying
- `fsync()` before replacement
- Atomic `rename()`
- File permission preservation
- File timestamp preservation
- Partial-write handling
- Interrupted system-call handling
- Exclusive backup locking using `flock()`
- Optional mirror deletion using `--delete`
- Recursive stale-directory deletion
- File-to-directory type-change handling
- Directory-to-file type-change handling
- Canonical path checking using `realpath()`
- Protection against unsafe nested source/backup paths
- Parent-child process architecture
- Anonymous pipe IPC
- Runtime statistics
- Recoverable error handling
- Automated regression testing
- Valgrind memory testing
- Helgrind concurrency testing
- `strace` system-call verification
- CO1-CO6 practical demonstrations

---

## Project Structure

```text
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
|   +-- worker_pool.c
|   +-- worker_pool.h
|   +-- fifo_demo.c
|   +-- signal_demo.c
|   +-- memory_demo.c
|   +-- thread_demo.c
|   +-- co1_syscall_demo.c
|   +-- co2_process_demo.c
|   +-- co3_ipc_demo.c
|   +-- co6_sync_demo.c
|
+-- tests/
|   +-- test_backup.sh
|
+-- Makefile
+-- .gitignore
+-- README.md
```

---

## Build Instructions

Build all components:

```bash
make clean && make
```

Build only the main backup application:

```bash
make backup_sync
```

Remove compiled binaries:

```bash
make clean
```

The main backup program is compiled with POSIX pthread support.

---

## Running the Backup Program

### Basic Backup

```bash
./backup_sync <source_directory> <backup_directory>
```

Example:

```bash
./backup_sync /tmp/source /tmp/backup
```

The default worker count is:

```text
4 threads
```

---

### Custom Worker Count

```bash
./backup_sync /tmp/source /tmp/backup --threads 8
```

Supported worker range:

```text
1 to 16 threads
```

---

### Mirror / Delete Mode

Deletion is disabled by default.

Normal mode:

```bash
./backup_sync /tmp/source /tmp/backup
```

retains files in the backup even if they have been deleted from the source.

Mirror mode:

```bash
./backup_sync /tmp/source /tmp/backup --delete
```

removes backup entries that no longer exist in the source.

---

### Threads and Delete Mode Together

```bash
./backup_sync /tmp/source /tmp/backup --threads 8 --delete
```

or:

```bash
./backup_sync /tmp/source /tmp/backup --delete --threads 8
```

---

## Synchronization Operations

Example output:

```text
[COPY] source/new.txt (NEW)

[UPDATE] source/report.txt (MODIFIED)

[SKIP] source/notes.txt (UNCHANGED)

[DELETE] backup/old.txt

[DELETE-DIR] backup/old_directory
```

---

## Metadata Comparison

The system compares source and backup files using:

- File size
- Modification-time seconds
- Modification-time nanoseconds

The implementation uses:

```c
st_mtim.tv_sec
st_mtim.tv_nsec
```

This allows rapid changes occurring within the same second to be detected.

Possible file states are:

```text
FILE_STATE_NEW
FILE_STATE_MODIFIED
FILE_STATE_UNCHANGED
FILE_STATE_ERROR
```

---

## Multithreaded Worker Pool

The actual backup engine uses POSIX threads.

The worker-pool implementation uses:

- `pthread_create()`
- `pthread_join()`
- Mutexes
- Condition variables
- Shared job queue
- Multiple concurrent copy workers
- Thread-safe statistics updates

New and modified files are placed into the worker queue and processed concurrently.

The number of workers can be configured using:

```bash
--threads N
```

where `N` must be between 1 and 16.

---

## Safe Atomic File Copying

New and modified files are first copied to a temporary file.

Workflow:

```text
Source file
    |
    v
Temporary backup file
    |
    v
read() / write()
    |
    v
Preserve permissions and timestamps
    |
    v
fsync()
    |
    v
rename()
    |
    v
Final backup file
```

Temporary files use names similar to:

```text
filename.tmp.XXXXXX
```

If copying fails, the temporary file is removed.

This prevents a partially written file from replacing a valid backup.

---

## Exclusive Backup Locking

Each backup directory uses:

```text
.backup_sync.lock
```

The application obtains a non-blocking exclusive lock using:

```c
flock(lock_fd, LOCK_EX | LOCK_NB)
```

If another cooperating backup process already holds the lock, the new process is rejected.

Example:

```text
[ERROR] Another backup process is already using:
        /tmp/backup
```

---

## Safe Delete Mode

Deletion occurs only when:

```bash
--delete
```

is explicitly supplied.

The deletion system supports:

- Stale regular files
- Stale directory trees
- Directory to file changes
- File to directory changes

The internal file:

```text
.backup_sync.lock
```

is excluded from deletion.

---

## Path Safety

Before synchronization begins, paths are resolved using:

```c
realpath()
```

The application rejects:

- Source and backup being the same directory
- Backup being inside source
- Source being inside backup
- Unsafe paths using `..`
- Unsafe symbolic-link path aliases

This prevents recursive self-backup and dangerous deletion behavior.

---

## Parent-Child Process Architecture

The parent process:

1. Validates command-line arguments
2. Validates source directory
3. Creates the backup root if necessary
4. Validates source and backup path relationships
5. Acquires the backup lock
6. Creates an anonymous pipe
7. Calls `fork()`
8. Waits for the child using `waitpid()`
9. Reads the final synchronization status

The child process:

1. Creates the worker pool
2. Performs optional mirror deletion
3. Recursively scans the source
4. Submits copy jobs
5. Waits for workers to finish
6. Prints synchronization statistics
7. Sends final status through the pipe

Possible IPC messages:

```text
SYNC_SUCCESS
SYNC_COMPLETED_WITH_ERRORS
```

---

## Runtime Statistics

The program displays:

```text
Files scanned
New files copied
Files updated
Unchanged skipped
Directories created
Files deleted
Directories deleted
Bytes copied
Worker threads
Elapsed time
Errors
```

Example:

```text
========================================
 Synchronization Summary
========================================
Files scanned       : 20
New files copied    : 4
Files updated       : 2
Unchanged skipped   : 14
Directories created : 1
Files deleted       : 0
Directories deleted : 0
Bytes copied        : 4096
Worker threads      : 4
Elapsed time        : 0.012 seconds
Errors              : 0
========================================
```

---

## Error Handling

Recoverable errors do not immediately stop the entire backup.

For example, if one source file cannot be opened:

```text
Error opening source file: Permission denied
[ERROR] Failed to copy new file: ...
```

other accessible files can still be copied.

The final result still correctly reports:

```text
Errors              : 1
Parent received: SYNC_COMPLETED_WITH_ERRORS
Child exit status: 1
```

---

## Automated Regression Testing

Run:

```bash
./tests/test_backup.sh
```

The final regression suite contains **14 major test groups**:

1. Initial recursive backup
2. Unchanged-file detection
3. Nanosecond modification detection
4. Deep recursive synchronization
5. Worker-thread and byte statistics
6. Invalid thread-count rejection
7. Safe optional `--delete` behavior
8. Stale directory-tree deletion
9. File/directory type changes
10. Dangerous source/backup path protection
11. Exclusive backup locking
12. Threaded recoverable error handling
13. Atomic temporary-file cleanup
14. Invalid source rejection

Final verified result:

```text
========================================
 ALL FINAL REGRESSION TESTS PASSED
========================================
```

---

## Valgrind Memory Verification

The final threaded backup engine was tested using Valgrind Memcheck.

Result for both the parent and child processes:

```text
All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors
```

Valgrind exit status:

```text
0
```

---

## Helgrind Concurrency Verification

The worker pool was tested using Valgrind Helgrind with four worker threads and twenty real copy jobs.

Result:

```text
ERROR SUMMARY: 0 errors from 0 contexts
```

Helgrind exit status:

```text
0
```

No tested data races or synchronization errors were detected.

---

## strace System-Call Verification

The final program was verified using Linux `strace`.

Observed operations included:

```text
flock(...)
pipe2(...)
clone(... SIGCHLD ...)
clone3(... CLONE_THREAD ...)
mkdir(...)
utimensat(...)
fsync(...)
rename(...)
write(... "SYNC_SUCCESS" ...)
wait4(...)
read(... "SYNC_SUCCESS" ...)
```

This confirms that the project uses real Linux kernel services.

---

# Course Outcome Mapping

## CO1 - Operating System as a Service Layer

Implemented primarily in:

```text
src/co1_syscall_demo.c
```

Demonstrates:

- Linux system-call interface
- Direct `syscall()` usage
- `SYS_getpid`
- `SYS_write`
- User-space to kernel-space service requests
- System-call verification using `strace`

Run:

```bash
./co1_syscall_demo
```

---

## CO2 - Unix Process Control

Implemented in:

```text
src/co2_process_demo.c
src/main.c
```

Demonstrates:

- `fork()`
- `exec()` using `execl()`
- `waitpid()`
- `_exit()`
- Parent-child process creation
- Process replacement
- Process synchronization
- Child-process reaping

Run:

```bash
./co2_process_demo
```

---

## CO3 - Inter-Process Communication

Implemented through:

```text
src/main.c
src/fifo_demo.c
src/signal_demo.c
src/co3_ipc_demo.c
```

Demonstrates:

- Anonymous pipes
- Named pipes / FIFO
- Unix-domain sockets
- Shared memory
- Signals
- `SIGUSR1`
- Parent-child communication

Run:

```bash
./fifo_demo
./signal_demo
./co3_ipc_demo
```

---

## CO4 - Virtual Memory

Implemented in:

```text
src/memory_demo.c
```

Demonstrates:

- `mmap()`
- Virtual memory allocation
- System page-size detection
- `fork()`
- Copy-on-Write
- Parent/child memory isolation
- `munmap()`

Run:

```bash
./memory_demo
```

---

## CO5 - File Systems and File I/O

Implemented primarily through:

```text
src/main.c
src/file_ops.c
src/metadata.c
src/sync.c
src/worker_pool.c
```

Demonstrates:

- File descriptors
- `open()`
- `read()`
- `write()`
- `close()`
- `stat()`
- `lstat()`
- `fstat()`
- `futimens()`
- `fsync()`
- `rename()`
- `unlink()`
- `mkdir()`
- `rmdir()`
- `opendir()`
- `readdir()`
- `closedir()`
- Recursive directory traversal
- Metadata comparison
- Atomic file replacement
- Optional deletion synchronization

Run:

```bash
./backup_sync <source_directory> <backup_directory>
```

---

## CO6 - Concurrency and Synchronization

Implemented in:

```text
src/worker_pool.c
src/thread_demo.c
src/co6_sync_demo.c
```

Demonstrates:

- POSIX threads
- Real concurrent file copying
- `pthread_create()`
- `pthread_join()`
- Mutexes
- Condition variables
- Shared worker queue
- Thread-safe shared statistics
- Race-condition prevention
- Counting semaphores in the dedicated CO6 demonstration

Run:

```bash
./backup_sync /tmp/source /tmp/backup --threads 4
./thread_demo
./co6_sync_demo
```

---

## Additional Demonstrations

### FIFO

Run:

```bash
./fifo_demo
```

The FIFO is created at:

```text
/tmp/ossp_backup_fifo
```

`/tmp` is used because Unix FIFOs may not work correctly on Windows-mounted WSL directories such as `/mnt/c`.

### Signal Handling

Run:

```bash
./signal_demo
```

The parent sends `SIGUSR1` to the child.

### Advanced IPC

Run:

```bash
./co3_ipc_demo
```

Demonstrates Unix-domain sockets and shared anonymous memory.

---

## Current Limitations

- Synchronization is one-way from source to backup.
- Source symbolic-link entries are skipped rather than copied as symbolic links.
- Unsupported special file types are skipped.
- `flock()` is advisory and protects cooperating processes.
- Change detection is based on size and timestamps rather than content checksums.
- A source file modified while actively being copied is not a point-in-time filesystem snapshot.
- Atomic replacement calls `fsync()` on the temporary file before `rename()`, but does not additionally `fsync()` the containing directory.
- The worker-job queue is dynamically allocated and currently unbounded.

---

## Possible Future Enhancements

- Optional checksum verification
- SHA-256 integrity verification
- Persistent log files
- Symbolic-link backup support
- Backup version history
- Scheduled backups
- Bounded worker queue
- Filesystem snapshot support
- Configuration-file support
- Graphical user interface

---

## Development Environment

- Programming Language: C
- Platform: Linux / Ubuntu / WSL
- Compiler: GCC
- Build Tool: Make
- Threading: POSIX pthreads
- Automated Testing: Bash
- Memory Analysis: Valgrind Memcheck
- Concurrency Analysis: Valgrind Helgrind
- System-Call Analysis: `strace`

---

## Project Status

**Implementation complete and final regression testing successful.**

The project provides a functional Linux backup and synchronization application while demonstrating practical Operating Systems concepts across **CO1 through CO6**.
```

After pasting into Nano:

```text
Ctrl + O
Enter
Ctrl + X
```

Then run:

```bash
git status --short README.md
git diff --check
```

You should see:

```text
 M README.md
```

and `git diff --check` should produce no output.
