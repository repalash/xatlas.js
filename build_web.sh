#!/bin/bash

set -e

export OPTIMIZE="-Os"
export LDFLAGS="${OPTIMIZE}"
export CFLAGS="${OPTIMIZE}"
export CPPFLAGS="${OPTIMIZE}"

ENVIRONMENT=${ENVIRONMENT:-'web,worker'}

echo "============================================="
echo "Compiling wasm bindings"
echo "============================================="
(

  mkdir -p source/web/build

  # Compile C/C++ code
  emcc \
    -std=c++1y \
    -DXA_MULTITHREADED=0 \
    ${OPTIMIZE} \
    --bind \
    --no-entry \
    -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MALLOC=emmalloc \
    -s MODULARIZE=1 \
    -s EXPORT_ES6=0 \
    -s ENVIRONMENT="${ENVIRONMENT}" \
    -s EXPORT_NAME="createXAtlasModule" \
    -o ./source/web/build/xatlas.js \
    --js-library ./source/web/jslib.js \
    source/web/*.cpp \
    source/xatlas/xatlas.cpp \
\
    -s ASSERTIONS=1 \
    -DNDEBUG \
#    -s TOTAL_MEMORY=278mb \
#    -D SANITIZE_ADDRESS_CHECK \
#    -fsanitize=address \
#    -g3
#    Uncomment above line for leak checking

  # Move artifacts
  if [ "$ENVIRONMENT" == "node" ]; then
    rm -rf dist/node
    mkdir -p dist/node
    mv source/web/build/xatlas.wasm dist/node
    mv source/web/build/xatlas.js dist/node
    cp source/web/api.mjs dist/node
    cp source/web/node.worker.mjs dist/node/worker.mjs
  else
    rm -rf dist
    mkdir -p dist
    mv source/web/build/xatlas.wasm dist
  fi
  #  mv source/web/build/xatlas.wasm.map dist

)
echo "============================================="
echo "Compiling wasm bindings done"
echo "============================================="
