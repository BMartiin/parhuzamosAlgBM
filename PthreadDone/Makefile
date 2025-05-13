
CC = gcc

CFLAGS = -Wall -O2 -pthread -lrt

TARGET = main
SRC = main.c

# Alapértelmezett cél
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Takarítás
clean:
	rm -f $(TARGET) *.pgm performance.csv
