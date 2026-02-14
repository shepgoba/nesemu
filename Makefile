CC = gcc
BINARY_NAME = nesmu

SRC_DIR := src
OBJ_DIR := obj

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

CXXFLAGS += -Og -Wall -Wextra -Wpedantic -Wno-unused -Wno-unused-parameter -std=c23
LDFLAGS += -lSDL3

ROM_FILE = roms/mario.nes

build/$(BINARY_NAME): $(OBJ_FILES)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<


run: build/$(BINARY_NAME)
	./build/$(BINARY_NAME) $(ROM_FILE)

do: build/$(BINARY_NAME) run

clean:
	@rm -rf obj/*.o
	@rm -rf build/*
