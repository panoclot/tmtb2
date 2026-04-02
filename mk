#!/bin/sh

mkdir -p ./build/db
touch ./build/config
gcc -o build/tt tt.c -Wall -Wextra -std=c99 -pedantic

if [ "$1" = "run" ]; then
	./build/tt
fi
