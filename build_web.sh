#!/bin/bash

set -e

export OPTIMIZE="-Os"
export LDFLAGS="${OPTIMIZE}"
export CFLAGS="${OPTIMIZE}"
export CPPFLAGS="${OPTIMIZE}"

echo "============================================="
echo "Compiling wasm bindings"
echo "============================================="
(

  mkdir -p source/web/build

  # Compile C/C++ code
  emcc \
    -std=c++1y \
    -DXA_MULTITHREADED=0 \
    -DNDEBUG \
    ${OPTIMIZE} \
    --bind \
    --no-entry \
    -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MALLOC=emmalloc \
    -s MODULARIZE=1 \
    -s ENVIRONMENT='web,worker' \
    -s ASSERTIONS=1 \
    -s EXPORT_NAME="createXAtlasModule" \
    -o ./source/web/build/xatlas_web.js \
    source/web/*.cpp \
    source/xatlas/xatlas.cpp \
\
#    -s TOTAL_MEMORY=278mb \
#    -D SANITIZE_ADDRESS_CHECK \
#    -fsanitize=address \
#    -g2
#    Uncomment above line for leak checking

  # Move artifacts
  rm -rf dist
  mkdir -p dist
  mv source/web/build/xatlas_web.wasm dist

#  cp dist/xatlas_web.wasm /Users/palash/Documents/Projects/webgl-lightmapper/public/xatlas_web.wasm
#  cp dist/xatlas_web.js /Users/palash/Documents/Projects/webgl-lightmapper/src/xatlas/xatlas_web.js
)
echo "============================================="
echo "Compiling wasm bindings done"
echo "============================================="
