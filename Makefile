CC = gcc

SRC_DIR := src

OBJ_DIR := obj
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

CXXFLAGS += -O3 -Wall -Wextra -Wpedantic -Wno-unused -Wno-unused-parameter -std=c23
LDFLAGS += -lSDL3


ROM_FILE = roms/mario.nes

nesemu: $(OBJ_FILES)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<


run: nesemu
	./nesemu $(ROM_FILE)

do: nesemu run

clean:
	@rm -rf obj/*.o
	rm nesemu
	rm nesemu.exe
