TARGET = echos_of_cinders
CC = gcc
CFLAGS = -Wall -std=c99 -Wno-missing-braces -O2
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Lista todos os arquivos .c
SRCS = $(wildcard *.c)
# Transforma a lista de .c em .o
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f *.o $(TARGET)

run: all
	./$(TARGET)

# Adicionei este comando para limpar e rodar tudo do zero
rebuild: clean run