TARGET = echos_of_cinders

CC = gcc

CFLAGS = -Wall -std=c99 -Wno-missing-braces

LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: $(TARGET)

$(TARGET): *.c
	$(CC) *.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)