# cliente/Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = client
SRC = client.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
