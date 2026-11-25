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
HDR     = $(wildcard include/*.h)
CFLAGS  = -g -Wall -Wextra -std=c99 -Wno-missing-braces -I./include -I./helloRaylib/include

ifeq ($(OS),Windows_NT)
	LDFLAGS = -L./helloRaylib/libwin
	LDLIBS  = -lraylib -lopengl32 -lgdi32 -lwinmm -lm
	BIN     = $(BINNAME).exe
	# A libraylib.a do Windows e 64 bits. Se o "gcc" achado no PATH for
	# de 32 bits (ex.: o MinGW antigo em C:\MinGW), o link falha com
	# "undefined reference". Nesse caso usa o gcc 64 bits do w64devkit.
	W64GCC = C:/raylib/w64devkit/bin/gcc.exe
	ifeq ($(findstring x86_64,$(shell $(CC) -dumpmachine)),)
		ifneq ($(wildcard $(W64GCC)),)
			CC = $(W64GCC)
		else
			$(error O gcc do PATH nao e 64 bits. Instale o w64devkit em C:\raylib ou coloque um gcc 64 bits no PATH)
		endif
	endif
else
	LDFLAGS = -L./helloRaylib/lib
	LDLIBS  = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	BIN     = $(BINNAME)
endif

RM = rm -f
ifeq ($(OS),Windows_NT)
	# Sem "rm" no PATH (ex.: so o MinGW antigo, sem o w64devkit), usa o del do Windows
	ifeq ($(wildcard $(addsuffix /rm.exe,$(subst ;, ,$(subst \,/,$(PATH))))),)
		RM = del /Q /F
	endif
endif

.PHONY: all run debug clean valgrind doctor docs

# As bibliotecas (LDLIBS) vem DEPOIS dos fontes: ordem exigida pelo linker
all: $(BIN)

$(BIN): $(SRC) $(HDR)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $(BIN)

run: all
	./$(BIN)

debug: all
	gdb ./$(BIN)

clean:
	$(RM) $(BIN)

valgrind: all
	valgrind --tool=memcheck --leak-check=full --track-origins=yes --show-leak-kinds=all --show-reachable=yes ./$(BIN)

# Diagnostico: confere se o gcc e a libraylib.a sao compativeis (ambos 64 bits)
doctor:
	@echo Compilador:
	@$(CC) -dumpmachine
	@echo Fontes encontrados:
	@echo $(SRC)
	@echo Formato da libraylib.a:
	@objdump -f ./helloRaylib/libwin/libraylib.a | head -n 4

# Gera a documentacao Doxygen em docs/html (precisa do doxygen instalado)
docs:
	doxygen Doxyfile
