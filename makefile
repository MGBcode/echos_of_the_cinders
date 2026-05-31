TARGET_WIN := echos_of_cinders.exe
TARGET_UNIX := echos_of_cinders
CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -O2
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

ifeq ($(OS),Windows_NT)
TARGET := $(TARGET_WIN)
LDFLAGS := -lraylib -lopengl32 -lgdi32 -lwinmm
CLEAN_CMD := del /Q
else
TARGET := $(TARGET_UNIX)
LDFLAGS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
CLEAN_CMD := rm -f
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	$(CLEAN_CMD) *.o $(TARGET)

rebuild: clean run
