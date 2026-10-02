CC = gcc
CFLAGS = -Wall -Wextra -g

BACKUP_TARGET = backup_sync
BACKUP_SRC = src/main.c src/file_ops.c src/metadata.c src/sync.c

FIFO_TARGET = fifo_demo
FIFO_SRC = src/fifo_demo.c

SIGNAL_TARGET = signal_demo
SIGNAL_SRC = src/signal_demo.c

MEMORY_TARGET = memory_demo
MEMORY_SRC = src/memory_demo.c

THREAD_TARGET = thread_demo
THREAD_SRC = src/thread_demo.c

all: $(BACKUP_TARGET) $(FIFO_TARGET) $(SIGNAL_TARGET) $(MEMORY_TARGET) $(THREAD_TARGET)

$(BACKUP_TARGET): $(BACKUP_SRC)
	$(CC) $(CFLAGS) $(BACKUP_SRC) -o $(BACKUP_TARGET)

$(FIFO_TARGET): $(FIFO_SRC)
	$(CC) $(CFLAGS) $(FIFO_SRC) -o $(FIFO_TARGET)

$(SIGNAL_TARGET): $(SIGNAL_SRC)
	$(CC) $(CFLAGS) $(SIGNAL_SRC) -o $(SIGNAL_TARGET)

$(MEMORY_TARGET): $(MEMORY_SRC)
	$(CC) $(CFLAGS) $(MEMORY_SRC) -o $(MEMORY_TARGET)

$(THREAD_TARGET): $(THREAD_SRC)
	$(CC) $(CFLAGS) -pthread $(THREAD_SRC) -o $(THREAD_TARGET)

clean:
	rm -f $(BACKUP_TARGET) $(FIFO_TARGET) $(SIGNAL_TARGET) $(MEMORY_TARGET) $(THREAD_TARGET)

.PHONY: all clean
