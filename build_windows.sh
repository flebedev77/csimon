#!/bin/sh

x86_64-w64-mingw32-gcc main.c -o csimon.exe -L./libs/windows/rl -I./libs/windows/rl/include -lm -lpthread -lraylib -lgdi32 -lwinmm
