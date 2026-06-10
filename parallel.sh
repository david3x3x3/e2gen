#!/bin/bash
#seq 0 45 | xargs -P `nproc` -I {} sh -c 'echo running {}; ./7x7-row-3-172 {} {} > 7x7_{}.log 2>&1'
seq 0 69 | xargs -P `nproc` -I {} sh -c 'echo running {}; ./8x8-row-3-232 {} {} > 8x8_{}.log 2>&1'
