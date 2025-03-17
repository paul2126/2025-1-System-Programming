#!/bin/bash

# Get the filename without the .c extension
filename=$(basename "$1" .c)

# Flags for compilation (you can adjust these)
# compile_flags="-Wall -Werror -ansi -pedantic -std=c99"

echo "Preprocessing..."
gcc $compile_flags -E "$1" -o "$filename".i

echo "Compiling..."
gcc $compile_flags -S "$1" -o "$filename".s

echo "Assembling..."
gcc $compile_flags -c "$1" -o "$filename".o

echo "Linking..."
gcc "$filename".o -o "$filename"