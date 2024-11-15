#!/bin/bash -x
#for what in eternity2.spiral.1 eternity2.spiral.5 eternity2.row.1 eternity2.row.5; do
for what in eternity2.row.5 eternity2.row.1 eternity2.spiral.5 eternity2.spiral.1 10x10.row.0 10x10.spiral.0 6x6.row.0 7x7.row.0 8x8.row.0 9x9.row.0; do
    echo $what
    puzzle=$(echo $what | cut -d. -f1)
    method=$(echo $what | cut -d. -f2)
    hints=$(echo $what | cut -d. -f3)
    python3 spiral-gen.py $puzzle $method $hints
    name="${puzzle}-${method}"
    if [ $hints -gt 1 ]; then
	name="${name}-hints"
    fi

    # emcc -s BINARYEN=1 -s EXPORTED_FUNCTIONS=_main,_origmain -s EXPORTED_RUNTIME_METHODS=ccall -O2 -o ${name}-wasm.js e2.c
    gcc -O6 -o $name e2.c
    # x86_64-w64-mingw32-gcc -O6 -o $name e2.c
    # clang -O3 -o $name e2.c
done
