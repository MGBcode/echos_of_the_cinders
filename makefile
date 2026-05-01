# Nome do executável final
TARGET = echos_of_cinders

# Compilador
CC = gcc

# Opções de compilação (Warnings para pegar erros comuns)
CFLAGS = -Wall -std=c99 -Wno-missing-braces

# Bibliotecas necessárias para o Raylib no Linux/WSL
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Regra principal: Compilar tudo
all: $(TARGET)

$(TARGET): main.c
	$(CC) main.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)

# Regra para limpar os arquivos gerados
clean:
	rm -f $(TARGET)

# Regra para rodar o jogo direto
run: all
	./$(TARGET)