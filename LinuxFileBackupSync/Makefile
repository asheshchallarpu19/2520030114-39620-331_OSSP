CC = gcc
CFLAGS = -Wall -Wextra -g
THREAD_FLAGS = -pthread

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

CO1_TARGET = co1_syscall_demo
CO1_SRC = src/co1_syscall_demo.c

CO2_TARGET = co2_process_demo
CO2_SRC = src/co2_process_demo.c

CO3_TARGET = co3_ipc_demo
CO3_SRC = src/co3_ipc_demo.c

CO6_TARGET = co6_sync_demo
CO6_SRC = src/co6_sync_demo.c

all: \
	$(BACKUP_TARGET) \
	$(FIFO_TARGET) \
	$(SIGNAL_TARGET) \
	$(MEMORY_TARGET) \
	$(THREAD_TARGET) \
	$(CO1_TARGET) \
	$(CO2_TARGET) \
	$(CO3_TARGET) \
	$(CO6_TARGET)

$(BACKUP_TARGET): $(BACKUP_SRC)
	$(CC) $(CFLAGS) $(BACKUP_SRC) -o $(BACKUP_TARGET)

$(FIFO_TARGET): $(FIFO_SRC)
	$(CC) $(CFLAGS) $(FIFO_SRC) -o $(FIFO_TARGET)

$(SIGNAL_TARGET): $(SIGNAL_SRC)
	$(CC) $(CFLAGS) $(SIGNAL_SRC) -o $(SIGNAL_TARGET)

$(MEMORY_TARGET): $(MEMORY_SRC)
	$(CC) $(CFLAGS) $(MEMORY_SRC) -o $(MEMORY_TARGET)

$(THREAD_TARGET): $(THREAD_SRC)
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $(THREAD_SRC) -o $(THREAD_TARGET)

$(CO1_TARGET): $(CO1_SRC)
	$(CC) $(CFLAGS) $(CO1_SRC) -o $(CO1_TARGET)

$(CO2_TARGET): $(CO2_SRC)
	$(CC) $(CFLAGS) $(CO2_SRC) -o $(CO2_TARGET)

$(CO3_TARGET): $(CO3_SRC)
	$(CC) $(CFLAGS) $(CO3_SRC) -o $(CO3_TARGET)

$(CO6_TARGET): $(CO6_SRC)
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $(CO6_SRC) -o $(CO6_TARGET)

clean:
	rm -f \
	$(BACKUP_TARGET) \
	$(FIFO_TARGET) \
	$(SIGNAL_TARGET) \
	$(MEMORY_TARGET) \
	$(THREAD_TARGET) \
	$(CO1_TARGET) \
	$(CO2_TARGET) \
	$(CO3_TARGET) \
	$(CO6_TARGET)

.PHONY: all clean
