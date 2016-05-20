
#!/bin/sh

mkdir -p build
cd build

g++ -std=c++0x -g `readlink -f ../code/dracogen.cpp` `readlink -f ../code/linux_dracogen.cpp` -o dracogen -I/home/chronal/dev/lib/chr -Wno-write-strings -lrt

cd ..
