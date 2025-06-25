# RoboticLib/Makefile

CC = gcc
AR = ar
CFLAGS = -Wall -O2
TARGET = roboticLib.a
OBJS = roboticLib.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

roboticLib.o: roboticLib.c roboticLib.h
	$(CC) $(CFLAGS) -c roboticLib.c

clean:
	rm -f $(OBJS) $(TARGET)

