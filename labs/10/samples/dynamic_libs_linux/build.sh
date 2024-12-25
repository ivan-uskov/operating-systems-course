#!/usr/bin/env bash
set -e

g++ -c -fPIC ./libexample/src/example.cpp -o library.o

g++ -shared -o libexample.so library.o

g++ -I ./libexample/include -L . ./app/main.cpp -lexample -o myapp

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
./myapp