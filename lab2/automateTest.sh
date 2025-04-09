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

./src/dirtree ./tools/demo ./tools/test2/ > result9.mine 2>&1
./reference/dirtree ./tools/demo ./tools/test2/> result9.ref 2>&1
diff -c result9.mine result9.ref

./src/dirtree -v ./tools/demo ./tools/test2/> result10.mine 2>&1
./reference/dirtree -v ./tools/demo ./tools/test2/> result10.ref 2>&1
diff -c result10.mine result10.ref

./src/dirtree -t ./tools/demo ./tools/test2/> result11.mine 2>&1
./reference/dirtree -t ./tools/demo ./tools/test2/> result11.ref 2>&1
diff -c result11.mine result11.ref

./src/dirtree -s ./tools/demo ./tools/test2/> result12.mine 2>&1
./reference/dirtree -s ./tools/demo ./tools/test2/> result12.ref 2>&1
diff -c result12.mine result12.ref

./src/dirtree -v -t ./tools/demo ./tools/test2/> result13.mine 2>&1
./reference/dirtree -v -t ./tools/demo ./tools/test2/> result13.ref 2>&1
diff -c result13.mine result13.ref

./src/dirtree -v -s ./tools/demo ./tools/test2/> result14.mine 2>&1
./reference/dirtree -v -s ./tools/demo ./tools/test2/> result14.ref 2>&1
diff -c result14.mine result14.ref

./src/dirtree -s -t ./tools/demo ./tools/test2/> result15.mine 2>&1
./reference/dirtree -s -t ./tools/demo ./tools/test2/> result15.ref 2>&1
diff -c result15.mine result15.ref

./src/dirtree -s -t -v ./tools/demo ./tools/test2/> result16.mine 2>&1
./reference/dirtree -s -t -v ./tools/demo ./tools/test2/> result16.ref 2>&1
diff -c result16.mine result16.ref