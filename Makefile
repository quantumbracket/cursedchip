CC=g++
CFLAGS=-lncurses

TARGET=cursedchip


$(TARGET): $(TARGET).cpp
	mkdir -p build
	$(CC) $(CFLAGS) -o build/$(TARGET) $(TARGET).cpp

.PHONY: clean debug

debug:
	$(CC) -g $(CFLAGS) -o build/$(TARGET) $(TARGET).cpp



clean:
	rm -rf build
