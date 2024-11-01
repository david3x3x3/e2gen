#!/bin/bash -x
#for what in spiral.1 spiral.5 row.1 row.5; do
for what in spiral.5; do
    echo $what
    method=$(echo $what | cut -d. -f1)
    hints=$(echo $what | cut -d. -f2)
    python3 spiral-gen.py $method $hints
    name=$method
    if [ $hints -gt 1 ]; then
	name="${name}-hints"
    fi
    # emcc -s BINARYEN=1 -s EXPORTED_FUNCTIONS=_main,_origmain -s EXPORTED_RUNTIME_METHODS=ccall -O2 -o ${name}-wasm.js e2.c
    # gcc -O6 -o $name e2.c
    clang -O3 -o $name e2.c
done
