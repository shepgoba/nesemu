CC = gcc
BINARY_NAME = nesemu

SRC_DIR := src
OBJ_DIR := obj

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

CFLAGS += -Og -Wall -Wextra -Wpedantic -Wno-unused -Wno-unused-parameter -std=c23
LDFLAGS += -lSDL3

UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(UNAME_S),Linux)
	CFLAGS += -DNESEMU_LINUX
endif

ifeq ($(UNAME_S),Darwin)
	CFLAGS += -DNESEMU_MACOS
endif

ifeq ($(OS),Windows_NT)
	CFLAGS += -DNESEMU_WINDOWS
endif


ROM_FILE = roms/mario.nes

build/$(BINARY_NAME): $(OBJ_FILES)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<


run: build/$(BINARY_NAME)
	./build/$(BINARY_NAME) $(ROM_FILE)

do: build/$(BINARY_NAME) run

clean:
	@rm -rf obj/*.o
	@rm -rf build/*
