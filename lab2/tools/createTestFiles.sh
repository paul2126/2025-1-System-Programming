#!/bin/bash

base="./test_root"
mkdir -p "$base"

for i in $(seq -w 0 999); do
    level1="$base/dir_$i"
    mkdir -p "$level1"
    
    for j in $(seq -w 0 999); do
        subdir="$level1/subdir_$j"
        mkdir -p "$subdir"
        touch "$subdir/file.txt"
    done
done
