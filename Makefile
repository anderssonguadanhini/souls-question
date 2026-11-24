# Makefile do projeto flameChaos (raylib + C99)
# Use o gcc de 64 bits do w64devkit (C:\raylib\w64devkit) e rode: make

# Verifica o sistema operacional
ifdef OS
  OS := $(strip $(OS))
else
  OS := $(strip $(shell uname))
endif

CC      = gcc
BINNAME = flameChaos
SRC     = $(wildcard src/*.c)
CFLAGS  = -g -Wall -Wextra -std=c99 -Wno-missing-braces -I./include -I./helloRaylib/include

ifeq ($(OS),Windows_NT)
	LDFLAGS = -L./helloRaylib/libwin
	LDLIBS  = -lraylib -lopengl32 -lgdi32 -lwinmm -lm
	BIN     = $(BINNAME).exe
else
	LDFLAGS = -L./helloRaylib/lib
	LDLIBS  = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	BIN     = ./$(BINNAME)
endif

RM = rm -f

.PHONY: all run debug clean valgrind doctor

# As bibliotecas (LDLIBS) vem DEPOIS dos fontes: ordem exigida pelo linker
all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $(BIN)

run: all
	$(BIN)

debug: all
	gdb $(BIN)

clean:
	$(RM) $(BIN)

valgrind: all
	valgrind --tool=memcheck --leak-check=full --track-origins=yes --show-leak-kinds=all --show-reachable=yes $(BIN)

# Diagnostico: confere se o gcc e a libraylib.a sao compativeis (ambos 64 bits)
doctor:
	@echo Compilador:
	@$(CC) -dumpmachine
	@echo Fontes encontrados:
	@echo $(SRC)
	@echo Formato da libraylib.a:
	@objdump -f ./helloRaylib/libwin/libraylib.a | head -n 4