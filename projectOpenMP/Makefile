
CC = gcc

CFLAGS = -Wall -O2 -fopenmp

TARGET = main
SRC = main.c

# Alapértelmezett cél
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Takarítási cél
clean:
	rm -f $(TARGET) *.ppm
