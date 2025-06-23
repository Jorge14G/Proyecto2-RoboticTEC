# RoboticLib/Makefile

CC = gcc
AR = ar
CFLAGS = -Wall -O2
TARGET = roboticLib.a
OBJS = roboticLib.o
TEST = test

all: $(TARGET) $(TEST)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

roboticLib.o: roboticLib.c roboticLib.h
	$(CC) $(CFLAGS) -c roboticLib.c

$(TEST): test.c $(TARGET)
	$(CC) test.c -L. -l:$(TARGET) -o $(TEST)

clean:
	rm -f $(OBJS) $(TARGET) $(TEST)
