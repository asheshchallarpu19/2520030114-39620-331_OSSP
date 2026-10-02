CC = gcc
CFLAGS = -Wall -Wextra -g

BACKUP_TARGET = backup_sync
BACKUP_SRC = src/main.c src/file_ops.c src/metadata.c src/sync.c

FIFO_TARGET = fifo_demo
FIFO_SRC = src/fifo_demo.c

SIGNAL_TARGET = signal_demo
SIGNAL_SRC = src/signal_demo.c

all: $(BACKUP_TARGET) $(FIFO_TARGET) $(SIGNAL_TARGET)

$(BACKUP_TARGET): $(BACKUP_SRC)
	$(CC) $(CFLAGS) $(BACKUP_SRC) -o $(BACKUP_TARGET)

$(FIFO_TARGET): $(FIFO_SRC)
	$(CC) $(CFLAGS) $(FIFO_SRC) -o $(FIFO_TARGET)

$(SIGNAL_TARGET): $(SIGNAL_SRC)
	$(CC) $(CFLAGS) $(SIGNAL_SRC) -o $(SIGNAL_TARGET)

clean:
	rm -f $(BACKUP_TARGET) $(FIFO_TARGET) $(SIGNAL_TARGET)

.PHONY: all clean
