#!/bin/sh

emcc main.c -o build/web/csimon.html -L./libs/web/rl -I./libs/web/rl/include -lraylib -s ALLOW_MEMORY_GROWTH=1 -s WASM=1 -s USE_GLFW=3 -s ASYNCIFY -s SINGLE_FILE=1
