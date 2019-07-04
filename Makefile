CC=g++
CFLAGS=-lncurses

TARGET=cursedchip


$(TARGET): source/main.cpp
	mkdir -p build
	$(CC) $(CFLAGS) -o build/$(TARGET) source/main.cpp

.PHONY: clean debug

debug:
	mkdir -p build
	$(CC) -g $(CFLAGS) -o build/$(TARGET) source/main.cpp



clean:
	rm -rf build
