CC = gcc

SRC_DIR := src

OBJ_DIR := obj
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

CXXFLAGS := -O3 -Wall -Wextra -Wno-unused
LDFLAGS :=
LIBS := -lSDL2 #-static  -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc -lsetupapi -lole32 -loleaut32 -lSDL2 


ROM_FILE = roms\donkeykong.nes

build\nesemu.exe: $(OBJ_FILES)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<


run: build\nesemu.exe
	.\build\nesemu.exe $(ROM_FILE)

do: build\nesemu.exe run

clean:
	@rm -rf obj/*.o
	@rm -rf build/*.exe