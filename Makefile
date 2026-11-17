# Verifica o sistema operacional
ifdef OS
  OS := $(strip $(OS))
else
  OS := $(strip $(shell uname))
endif

BINNAME = flameChaos
SRC = src/*.c

ifeq ($(OS),Windows_NT)
	INCLUDE = -I./include -I./helloRaylib/include -L./helloRaylib/libwin
	EXTRA_FLAGS = -Wall -Wextra -std=c99 -Wno-missing-braces -lraylib -lm -lopengl32 -lgdi32 -lwinmm
	BIN = $(BINNAME).exe
	RM = del /Q /F
else
	INCLUDE = -I./include -I./helloRaylib/include -L./helloRaylib/lib
	EXTRA_FLAGS = -Wall -Wextra -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	BIN = ./$(BINNAME)
	RM = rm -f
endif

all:
	gcc $(SRC) -g $(INCLUDE) $(EXTRA_FLAGS) -o $(BIN)

run: all
	$(BIN)

debug: all
	gdb $(BIN)

clean:
	$(RM) $(BIN)

valgrind: all
	valgrind --tool=memcheck --leak-check=full --track-origins=yes --show-leak-kinds=all --show-reachable=yes $(BIN)