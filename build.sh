#!/usr/bin/env bash

clang -o reader -g -Wall -Wextra `curl-config --cflags` main.c `curl-config --libs` -lxml2 -I/usr/include/libxml2
