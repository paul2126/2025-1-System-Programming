#!/bin/bash

# Test 
./src/dirtree ./tools/demo > result1.mine 2>&1
./reference/dirtree ./tools/demo > result1.ref 2>&1
diff -c result1.mine result1.ref

./src/dirtree -v ./tools/demo > result2.mine 2>&1
./reference/dirtree -v ./tools/demo > result2.ref 2>&1
diff -c result2.mine result2.ref

./src/dirtree -t ./tools/demo > result3.mine 2>&1
./reference/dirtree -t ./tools/demo > result3.ref 2>&1
diff -c result3.mine result3.ref

./src/dirtree -s ./tools/demo > result4.mine 2>&1
./reference/dirtree -s ./tools/demo > result4.ref 2>&1
diff -c result4.mine result4.ref

./src/dirtree -v -t ./tools/demo > result5.mine 2>&1
./reference/dirtree -v -t ./tools/demo > result5.ref 2>&1
diff -c result5.mine result5.ref

./src/dirtree -v -s ./tools/demo > result6.mine 2>&1
./reference/dirtree -v -s ./tools/demo > result6.ref 2>&1
diff -c result6.mine result6.ref

./src/dirtree -s -t ./tools/demo > result7.mine 2>&1
./reference/dirtree -s -t ./tools/demo > result7.ref 2>&1
diff -c result7.mine result7.ref

./src/dirtree -s -t -v ./tools/demo > result8.mine 2>&1
./reference/dirtree -s -t -v ./tools/demo > result8.ref 2>&1
diff -c result8.mine result8.ref