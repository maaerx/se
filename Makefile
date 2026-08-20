CXX = gcc
CSRC = main.c console.c
CFLAGS = -Iinclude

all:
	$(CXX) $(CSRC) $(CFLAGS) -o se.exe
