- `gcc800 -std=gnu99 testheapmgr.c heapmgr1.c chunk.c -o testheapmgr1` 형태로 맞춰줘야하기 때문에 tasks.json에다가 args로 다음과 같이 넣었다.
- 여러 c 파일을 컴파일 한 다음에 디버거에 붙히는 형태
```json
"args": [
    "-fdiagnostics-color=always",
    "-g",

    // "-std=c99",
    "-Wall",
    "-Werror",
    "-ansi",
    "-pedantic",
    "-std=gnu99",
    
    // "${file}",
    "/home/attat/snu/2025-1-System-Programming/lab3/test/testheapmgr.c",
    "/home/attat/snu/2025-1-System-Programming/lab3/src/heapmgr1.c",
    "/home/attat/snu/2025-1-System-Programming/lab3/src/chunk.c",
    "-o",
    // "${fileDirname}/${fileBasenameNoExtension}"
    "/home/attat/snu/2025-1-System-Programming/lab3/test/testheapmgr1"
],
```
- 이때 flag를 넣는 순서가 중요한데 -std=gnu99가 마지막으로 와야함. 왜냐하면 ansi가 "turn off GNU extensions and strictly follow ANSI C89" 의미를 가지고 있는데 -std=gnu99가 먼저 올 경우 ansi가 꺼버리기 때문
- launch.json에서 program 위치를 하드코딩 하고 args로 테스트에 필요한 값 변경해가면서 테스트 진행하면 된다