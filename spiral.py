#!/usr/bin/env python3
import random, time, sys

piece_data = """
bdaa/beaa/cdaa/dcaa/gbab/hcab/jbab/jfab/mdab/oeab/pcab/teab/tfab/veab/hbac/idac/
kfac/nfac/ocac/pcac/qeac/rbac/rfac/sbac/vbac/gcad/gdad/hdad/idad/ofad/pcad/sdad/
tcad/tead/ufad/wead/ibae/lfae/mfae/ncae/ndae/pbae/pcae/pdae/qbae/seae/teae/ueae/
gfaf/hbaf/hcaf/jbaf/oeaf/qeaf/qfaf/tdaf/ubaf/ufaf/vcaf/wdaf/jigg/kogg/hlgh/gtgi/
iwgi/kkgi/mhgi/sjgi/wtgi/logl/orgl/kigm/pqgm/spgm/tlgm/kpgn/npgn/lugo/slgo/uvgo/
nigp/iigq/mqgq/ingr/jkgr/trgr/usgr/gvgs/jwgs/qugs/mrgt/npgt/nqgt/qvgt/rkgt/vlgv/
qngw/stgw/vjgw/wvgw/rphh/ruhh/unhh/wjhh/qphi/lthj/nthj/qphj/umhj/prhk/rphk/snhl/
suhl/kqhm/orhm/rmhm/wuhm/lvhn/urho/twhp/kwhq/juhs/knhs/sphs/rwht/vpht/kjhu/nvhu/
qlhu/sthu/jshv/rkhv/kuhw/mqhw/qlhw/ushw/soii/jlij/jmij/jrij/nvij/pjij/urij/vvij/
lqik/iril/iwil/lkil/pril/jmin/qwin/mmio/mnio/wuio/ooiq/oriq/woiq/ouis/tjis/vvit/
jsiu/ksiu/pmiw/quiw/lqjk/qtjk/lljm/mpjn/nvjn/qtjo/sujo/vmjo/ppjp/rsjp/ovjq/onjr/
kqjs/rujt/tpjt/ouju/mujv/omjv/ntkk/wokl/nrkm/ttkm/vokn/kvko/lnko/unko/llkp/pskp/
mokq/vmkr/vwkr/wpkr/mwks/ntks/ssks/nlkt/vtku/woku/rnkv/rtkv/ulkv/wvkv/woll/nwlm/
tplm/mnlo/uplo/lulp/ttlq/lslr/qwlr/wvlr/npls/qrlu/mqlv/mulw/vvlw/vwlw/rwmm/somm/
upmm/npmo/rrmo/ntmq/owms/utms/rsmt/rvmt/nsnn/uqno/oqnq/osnq/rpnq/qunr/pwns/vpns/
ovnt/qvnt/prop/stop/wsoq/uwov/vwpp/rqpq/qvpr/uvps/wtqq/trqr/usrt/vusu/uwsw/vwtw
"""

# targets with rectangle interior
# 0 16 31 46 60 74 87 100 112 124 135 146 156 166 175 184 192 200 207
# 214 220 226 231 236 240 244 247 250 252 254 255 256

# targets with centered square interior
#  0 60 112 156 192 220 240 252
# square sizes
# 16 14  12  10   8   6   4   2

width, height = (16,16)
pieces = dict()
piecenum=1

for s in ''.join(piece_data.split('\n')).split('/'):
    for rot in range(4):
        pieces[(piecenum,rot)] = s[-rot:] + s[:-rot]
    piecenum += 1

keytypes = tuple(tuple(c == '1' for c in f'{x:04b}') for x in range(16))
fit = dict()
for key1, val in pieces.items():
    # there are 16 key types based on which of the 4 edges are relevant to the matching
    for kt in keytypes:
        key2 = []
        ok = True
        for i in range(4):
            if kt[i]:
                key2 += val[i]
            else:
                key2 += [None]
                if val[i] == 'a':
                    # we will never have border edges against unknown pieces
                    ok = False
        if ok:
            key2 = tuple(key2)    
            fit[key2] = fit.get(key2, []) + [key1]
# for k in fit:
#     print(k, fit[k])
# sys.exit(0)
hints = { 135: (139,2), 34: (208,3), 45: (255,3), 210: (181, 3), 221: (249, 0) }
# hints = { 135: (139,2) }
#hints = {}
dirs = (-1, 0), (0, 1), (1, 0), (0, -1)

solcount = 0

def build_spiral():
    global pos2ord, ord2pos
    r1 = 0
    c1 = 0
    i = 0
    dir = 0
    pos2ord = [-1]*(width*height)
    ord2pos_ = [-1]*(width*height)
    ord2pos = []
    # build spiral without hints
    while i < width*height:
        ord2pos_[i] = r1*width+c1
        pos2ord[r1*width+c1] = i
        r2 = r1 + dirs[dir][0]
        c2 = c1 + dirs[dir][1]
        if r2 < 0 or r2 >= height or \
           c2 < 0 or c2 >= width or \
           pos2ord[r2*width+c2] >= 0:
            dir = (dir+1)%4
        r1 = r1 + dirs[dir][0]
        c1 = c1 + dirs[dir][1]
        i += 1
    # redo the order with just the hint pieces
    for pos1 in hints:
        ord2pos += [pos1]
    # copy locations from the original order to the new order if they aren't hints
    for pos1 in ord2pos_:
        if pos1 not in hints:
            ord2pos += [pos1]
    # build ord2pos from pos2ord
    for ord1, pos1 in enumerate(ord2pos):
        pos2ord[pos1] = ord1

def print_puzz(max_ord):
    print(f'\n{max_ord} at {nodes}')
    for pos1 in range(width*height):
        ord1 = pos2ord[pos1]
        if ord1 < max_ord:
            print('%3d/%d' % Q[ord1][0], end=' ')
        else:
            print('  0/0', end=' ')
        if pos1 % width == (width-1):
            print('', flush=True)
    

def restart():
    global Q, placed, start_time, pos1, nodes, best, rate
    # print('restarting')
    nodes = 0
    best = 0
    rate = 10000
    start_time = int(time.time()-1)
    for k in fit:
        random.shuffle(fit[k])
    placed = [False]*(width*height+1)
    Q = [None]*(width*height)
    # place the hint pieces
    for i, pos1 in enumerate(hints):
        Q[i] = [hints[pos1]]
        placed[hints[pos1][0]] = True
    # print('.', end='', flush=True)

def speed_report():
    t = int(time.time())-start_time
    n = nodes
    e = 0
    while n >= 10000:
        n //= 1000
        e += 1
    e = "" if e == 0 else 'kmgtpezy'[e-1]
    print(f'nodes = {n}{e}, best = {best}, rate={nodes/1000000/t:.2f}m/s        ', end='\r', flush=True)


build_spiral()
for row in range(height):
    for col in range(width):
        print(f'{pos2ord[row*width+col]:3d} ', end='')
        if col+1 == width:
            print('')
restart()

funcs = []
for ord1 in range(width*height):
    src = f'def func{ord1}():\n'
    pos1 = ord2pos[ord1]
    r1 = pos1 // width
    c1 = pos1 % width
    src += f'    global Q, placed, nodes, best, rate\n'
    # src += f'    print("order {ord1}")\n'
    src += f'    if {ord1} > best:\n'
    src += f'        best = {ord1}\n'
    src += f'        if {ord1} >= 92:\n'
    src += f'            print_puzz({ord1})\n'
    src +=  "            print(f'{27:c}]0;{best}{7:c}', flush=True, end='')\n"
    src += f'    if Q[{ord1}] == None:\n'
    src += f'        Q[{ord1}] = [None]\n'
    cond = []
    key = []
    for dirnum, dir in enumerate(dirs):
        r2 = r1 + dir[0]
        c2 = c1 + dir[1]
        if r2 < 0 or r2 >= height or \
           c2 < 0 or c2 >= width:
            # off puzzle
            key += ["'a'"]
        else:
            ord2 = pos2ord[r2*width+c2]
            if ord2 < ord1:
                # adjacent to placed piece
                key += [f"pieces[Q[{ord2}][0]][{(dirnum+2)%4}]"]
            else:
                # adjacent to future piece
                key += ["None"]
    src += f'        for p1 in fit.get(({", ".join(key)}), []):\n'
    src += f'            if placed[p1[0]]:\n'
    src += f'                continue\n'
    src += f'            Q[{ord1}] += [p1]\n'
    src += f'    if Q[{ord1}][0] != None:\n'
    src += f'        placed[Q[{ord1}][0][0]] = False\n'
    src += f'    Q[{ord1}] = Q[{ord1}][1:]\n'
    src += f'    if len(Q[{ord1}]) > 0:\n'
    src += f'        placed[Q[{ord1}][0][0]] = True\n'
    src += f'        nodes += 1\n'
    src += f'        if nodes % rate == 0:\n'
    # src += f'            if rate > 10000:\n'
    # src += f'            if (nodes == 20000):\n'
    # src += f'                if (best < 136):\n'
    # all 5 hints:
    # src += f'            if (nodes == 2000000 and best < 106) or (nodes == 100000000 and best < 106):\n'
    # src += f'            if (nodes == 1000000 and best < 90) or (nodes == 100000000 and best < 106):\n'
    # src += f'            if (nodes == 20000 and best < 106) or (nodes == 1000000 and best < 174) or (nodes == 2000000 and best < 192):\n'
    src += f'            if (nodes == 20000 and best < 106) or (nodes == 1000000 and best < 117):\n'
    src += f'                restart()\n'
    src += f'                return {len(hints)}\n'
    src += f'            if nodes == 20000:\n'
    src += f'                rate = 1000000\n'
    src += f'            elif nodes > 20000:\n'
    src += f'                speed_report()\n'
    src += f'        return {ord1+1}\n'
    src += f'    Q[{ord1}] = None\n'
    src += f'    return {ord1-1}\n'
    src += f'funcs += [func{ord1}]\n'
    print(src)
    exec(src)

pos1 = len(hints)
while pos1 >= 0:
    pos1 = funcs[pos1]()

print(str(solcount) + ' solutions')
print(str(nodes) + ' nodes')
