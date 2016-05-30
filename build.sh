
#!/bin/sh

mkdir -p build
cd build
mkdir -p fsm

g++ -std=c++0x -g \
    `readlink -f ../code/graphgen.cpp`  \
    `readlink -f ../code/graphgen_static_posix.cpp`  \
    `readlink -f ../code/render.cpp`  \
    `readlink -f ../code/nfa_parse.cpp` \
    -o graphgen -Wno-write-strings -lrt

mkdir -p data
cp ../data/* data/

cd ..
