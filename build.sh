#!/bin/bash -x
BUILD_TS=$(date +%s)
#for what in eternity2.spiral.1 eternity2.spiral.5 eternity2.row.1 eternity2.row.5; do
# for what in eternity2.row.5 eternity2.row.1 eternity2.spiral.5 eternity2.spiral.1 10x10.row.0 10x10.spiral.0 6x6.row.0 7x7.row.0 8x8.row.0 9x9.row.0; do
# for what in eternity2.row.1 eternity2.strip.1 eternity2.strip.5 eternity2.spiral.1 eternity2.spiral.5; do

s=3
puzz=""
while [ $s -lt 21 ]; do
    for t in row spiral; do
	p=$(printf %dx%d.$t.0 $s $s)
	puzz="$puzz $p"
    done
    s=$((s+1))
done
# for what in 10x10.spiral.0 10x10.row.0 18x18.spiral.0 18x18.row.0 20x20.spiral.0 20x20.row.0; do
# for what in 9x9.row.0.7 10x10.row.0.10; do
for what in eternity2.row.1.7; do
    echo $what
    puzzle=$(echo $what | cut -d. -f1)
    method=$(echo $what | cut -d. -f2)
    hints=$(echo $what | cut -d. -f3)
    rowsize=$(echo $what | cut -d. -f4)
    cache=".numrows-cache-${puzzle}-${rowsize}"
    if [ -f "$cache" ]; then
        numrows=$(cat "$cache")
    else
        numrows=$(python3 et2.py --puzzle $puzzle --rowsize $rowsize | grep 'solutions$' | cut -d' ' -f1)
        echo "$numrows" > "$cache"
    fi
    python3 spiral-gen.py --puzzle $puzzle --method $method --hints $hints --rowsize $rowsize
    name="${puzzle}-${method}-${rowsize}-${numrows}"
    if [ $hints -gt 1 ]; then
	name="${name}-hints"
    fi

    # emcc -s EXPORTED_FUNCTIONS=_main,_origmain \
    #      -s EXPORTED_RUNTIME_METHODS=ccall,FS \
    #      -s FORCE_FILESYSTEM=1 \
    #      -O2 -o html/${name}-wasm.js e2.c
    # sed -i "s|\"${name}-wasm.wasm\"|\"${name}-wasm.wasm?v=${BUILD_TS}\"|" html/${name}-wasm.js
    # x86_64-w64-mingw32-gcc -O6 -o $name e2.c
    gcc -O3 -o $name e2.c
    # clang -O3 -o $name e2.c
done

echo "var WASM_VERSION='${BUILD_TS}';" > html/wasm-version.js
