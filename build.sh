#!/bin/bash

CFLAGS="-O0 -g" #"-O2"
gcc $CFLAGS -c main.c -o main.o \
&& gcc $CFLAGS -c tiny_fs.c -o tiny_fs.o \
&& gcc tiny_fs.o main.o -o test_tiny_fs
