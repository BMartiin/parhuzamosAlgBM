
CC = mpicc

CFLAGS = -Wall -O2

TARGET = mainMpi
SRC = mainMpi.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.csv
