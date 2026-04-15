CC = gcc
CFLAGS = $(shell pkg-config --cflags libxml-2.0)
LIBS = $(shell pkg-config --libs libxml-2.0)
TARGET = flightTool

all: $(TARGET)

$(TARGET): main.c
	$(CC) main.c -o $(TARGET) $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)