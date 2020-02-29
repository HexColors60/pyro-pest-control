#!/bin/sh

emmake make

cd build
emcc -g -O3 -s ALLOW_MEMORY_GROWTH=1 --closure 1 -flto --llvm-lto 1 --shell-file data/template.html --preload-file data rl.bc -o index.html
cd ..