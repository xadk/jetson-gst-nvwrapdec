#!/bin/sh
# deps: build-essential pkg-config libgstreamer1.0-dev
set -e
gcc -O2 -Wall -shared -fPIC gstnvwrapdec.c \
    $(pkg-config --cflags --libs gstreamer-1.0) \
    -o libgstnvwrapdec.so
echo "built libgstnvwrapdec.so"
