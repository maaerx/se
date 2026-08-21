CC = gcc
CFLAGS = -Iinclude
SRCS = main.c console.c

ifeq ($(OS),Windows_NT)
    TARGET = se.exe
else
    TARGET = se
endif

all:
	$(CC) $(SRCS) $(CFLAGS) -o $(TARGET)
