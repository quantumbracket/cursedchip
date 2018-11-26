CC=g++
CFLAGS=-lncurses

TARGET=cursedchip


$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp


clean:
	rm $(TARGET)
