#!/bin/bash
echo row no hints
python3 spiral-gen.py row 1
emcc -s BINARYEN=1 -s EXPORTED_FUNCTIONS=_main,_origmain -s EXPORTED_RUNTIME_METHODS=ccall -O2 -o row-wasm.js e2.c
echo row hints
python3 spiral-gen.py row 5
emcc -s BINARYEN=1 -s EXPORTED_FUNCTIONS=_main,_origmain -s EXPORTED_RUNTIME_METHODS=ccall -O2 -o row-hints-wasm.js e2.c
echo spiral no hints
python3 spiral-gen.py spiral 1
emcc -s BINARYEN=1 -s EXPORTED_FUNCTIONS=_main,_origmain -s EXPORTED_RUNTIME_METHODS=ccall -O2 -o spiral-wasm.js e2.c
echo spiral hints
python3 spiral-gen.py spiral 5
emcc -s BINARYEN=1 -s EXPORTED_FUNCTIONS=_main,_origmain -s EXPORTED_RUNTIME_METHODS=ccall -O2 -o spiral-hints-wasm.js e2.c
python3 spiral-gen.py row 1
gcc -O6 -o e2 e2.c
