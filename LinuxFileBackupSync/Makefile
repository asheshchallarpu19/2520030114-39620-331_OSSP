CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = backup_sync

SRC = src/main.c src/file_ops.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
