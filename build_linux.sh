#!/bin/sh

gcc main.c -o build/linux/csimon -L./libs/linux/rl -I./libs/linux/rl/include -lraylib -ldl -lrt -lm -lpthread
