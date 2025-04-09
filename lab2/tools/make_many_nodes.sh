#!/bin/bash

# How many of each type to create
NUM_FILES=1000
NUM_LINKS=1000
NUM_SYMLINKS=1000
NUM_PIPES=1000
NUM_SOCKETS=1000

# Base directory
base="./test_nodes"
mkdir -p "$base"

echo "Creating $NUM_FILES regular files..."
for i in $(seq 1 $NUM_FILES); do
    echo "File $i" > "$base/file_$i.txt"
done

echo "Creating $NUM_LINKS hard links..."
for i in $(seq 1 $NUM_LINKS); do
    ln "$base/file_$i.txt" "$base/hardlink_$i.txt"
done

echo "Creating $NUM_SYMLINKS symlinks..."
for i in $(seq 1 $NUM_SYMLINKS); do
    ln -s "$base/file_$i.txt" "$base/symlink_$i.txt"
done

echo "Creating $NUM_PIPES named pipes..."
for i in $(seq 1 $NUM_PIPES); do
    mkfifo "$base/pipe_$i"
done

echo "Creating $NUM_SOCKETS UNIX sockets..."
for i in $(seq 1 $NUM_SOCKETS); do
    socket_path="$base/socket_$i"
    [ -e "$socket_path" ] && rm -f "$socket_path"
    python3 -c "import socket as s; sock = s.socket(s.AF_UNIX); sock.bind('$socket_path')" &
    sleep 0.01
    kill $! >/dev/null 2>&1
done

echo "✅ All nodes created in: $base"
