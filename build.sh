#!/bin/bash -x
for what in spiral.1 spiral.5 row.1 row.5; do
# for what in spiral.5; do
    echo $what
    method=$(echo $what | cut -d. -f1)
    hints=$(echo $what | cut -d. -f2)
    python3 spiral-gen.py $method $hints
    name=$method
    if [ $hints -gt 1 ]; then
	name="${name}-hints"
    fi
    emcc -s BINARYEN=1 -s EXPORTED_FUNCTIONS=_main,_origmain -s EXPORTED_RUNTIME_METHODS=ccall -O2 -o ${name}-wasm.js e2.c
done

# echo row hints
# python3 spiral-gen.py row 5
# emcc -s BINARYEN=1 -s EXPORTED_FUNCTIONS=_main,_origmain -s EXPORTED_RUNTIME_METHODS=ccall -O2 -o row-hints-wasm.js e2.c
# echo spiral no hints
# python3 spiral-gen.py spiral 1
# emcc -s BINARYEN=1 -s EXPORTED_FUNCTIONS=_main,_origmain -s EXPORTED_RUNTIME_METHODS=ccall -O2 -o spiral-wasm.js e2.c
# echo spiral hints
# python3 spiral-gen.py spiral 5
# emcc -s BINARYEN=1 -s EXPORTED_FUNCTIONS=_main,_origmain -s EXPORTED_RUNTIME_METHODS=ccall -O2 -o spiral-hints-wasm.js e2.c
# python3 spiral-gen.py row 1
# gcc -O6 -o e2 e2.c
