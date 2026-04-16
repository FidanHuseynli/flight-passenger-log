CC = gcc
# Kütüphane yollarını pkg-config ile alıyoruz
CFLAGS = $(shell pkg-config --cflags libxml-2.0)
LIBS = $(shell pkg-config --libs libxml-2.0)

all:
	$(CC) main.c -o flightTool $(CFLAGS) $(LIBS)

clean:
	rm -f flightTool