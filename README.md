# What's this

FNF SDL SHOWCASE made in c++ and SDL3, it is optimized for performance and made for linux

cmd to compile: `g++ main.cpp simdjson.cpp -Iinclude -L. -lSDL3 -O3 -march=native -fopenmp -funroll-loops -fprefetch-loop-arrays -fno-strict-aliasing -ffast-math -Wall -flto=auto -fdata-sections -Wl,--gc-sections -fopenmp -o main`
