#!/bin/bash

for i in $(seq 0 5)
do
    ./reference/sampledecomment < ./test_files/test${i}.c > output1 2> errors1
    ./decomment < ./test_files/test${i}.c > output2 2> errors2
    diff -c output1 output2
    diff -c errors1 errors2
done

./reference/sampledecomment < ./src/decomment.c > output1 2> errors1
./decomment < ./src/decomment.c > output2 2> errors2

rm output1 errors1 output2 errors2