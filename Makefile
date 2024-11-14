CC := g++

CFLAGS := `sdl2-config --cflags` 
LDFLAGS := `sdl2-config --libs` -I/usr/include/SDL2 -lm -lSDL2_ttf -lSDL2_mixer -lSDL2_image
EXTRA_FLAGS := 

HDRS :=

# SRCS := $(wildcard ./*.cpp)
SRCS := $(shell find EngineOfEvil/source/ -name '*.cpp')

OBJS := $(patsubst %.cpp,%.o,${SRCS})

EXEC := game

# Emscripten
EMCC := EMSDK_PYTHON=/usr/bin/python3 emcc
EMSCRIPTEN_ACTIVATE_SCRIPT := /opt/ye/WebAssembly/emsdk-main/emsdk_env.sh

EM_BUILD_DIR := dist

# -I/opt/ye/WebAssembly/emsdk-main/upstream/emscripten/cache/sysroot/include/SDL
EMFLAGS := -I./EngineOfEvil/source \
					-I/opt/ye/WebAssembly/emsdk-main/upstream/emscripten/cache/sysroot/include/SDL2

EMLFLAGS := -sUSE_SDL=2 -sUSE_SDL_TTF=2 -sUSE_SDL_IMAGE=2 -sUSE_SDL_MIXER=2 \
					-sALLOW_MEMORY_GROWTH --embed-file asset_dir@/ 

#--preload-file asset_dir@/

# default recipe
all: compile_run

compile_run: compile

compile: $(OBJS) $(HDRS) Makefile
	$(CC) -o Builds/Release/$(EXEC) $(OBJS) $(LDFLAGS) $(EXTRA_FLAGS)

debug:
	$(MAKE) EXTRA_FLAGS='-ggdb3 -O0 -fsanitize=address,undefined'

${OBJS}:%.o : %.cpp
	${CC} -c $< -o $@ $(CFLAGS) $(EXTRA_FLAGS)

# %.o: %.cpp $(HDRS) Makefile
# 	$(CC) -o $@ -c $< $(CFLAGS) $(EXTRA_FLAGS)

clean:
	rm -f $(EXEC) $(OBJS)
	rm -rf $(EM_BUILD_DIR)

wasm_build_dir:
	mkdir -p $(EM_BUILD_DIR)

# source $(EMSCRIPTEN_ACTIVATE_SCRIPT)
# cp index.html $(EM_BUILD_DIR)

wasm: $(OBJS)
	EMCC_DEBUG=0
	$(EMCC) $(OBJS) $(EMFLAGS) $(EMLFLAGS) -o $(EM_BUILD_DIR)/index.html 

${OBJS}:%.o : %.cpp
	${EMCC} -c $< -o $@ $(EMFLAGS)


.PHONY: all clean compile compile_run debug wasm wasm_serve wasm_build_dir publish
