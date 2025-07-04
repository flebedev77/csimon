#!/bin/sh

mkdir -p build/linux
mkdir -p build/windows
mkdir -p build/web

echo "Building target [linux]"
./build_linux.sh > /dev/null
echo "Building target [windows]"
./build_windows.sh > /dev/null
echo "Building target [web]"
./build_web.sh > /dev/null
echo "Finished."
