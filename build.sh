#!/bin/bash
python3 spiral-gen.py
emcc -s BINARYEN=1 -s EXPORTED_FUNCTIONS=_main,_origmain -s EXPORTED_RUNTIME_METHODS=ccall -O2 -o e2-wasm.js e2.c
