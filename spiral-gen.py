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

hints = ((135, (139,2)), (34, (208,3)), (45, (255,3)), 
         (210, (181, 3)), (221, (249, 0)))
num_hints = int(sys.argv[2])
hints_dict = { k: v for k, v in hints[:num_hints] }
# hints = { 135: (139,2) }
#hints = {}
dirs = (-1, 0), (0, 1), (1, 0), (0, -1)

solcount = 0

def build_rowscan():
    global pos2ord, ord2pos, num_hints
    r1 = 0
    c1 = 0
    i = 0
    dir = 0
    pos2ord = [-1]*(width*height)
    ord2pos_ = [-1]*(width*height)
    ord2pos = []
    # build rowscan without hints
    ord2pos_ = [x for x in range(width*height)]
    # redo the order with just the hint pieces
    for pos1 in hints_dict:
        ord2pos += [pos1]
    # copy locations from the original order to the new order if they aren't hints
    for pos1 in ord2pos_:
        if pos1 not in hints_dict:
            ord2pos += [pos1]
    # build ord2pos from pos2ord
    for ord1, pos1 in enumerate(ord2pos):
        pos2ord[pos1] = ord1
    
def build_spiral():
    global pos2ord, ord2pos, num_hints
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

    # put 33 before 17, otherwise 33 will be boxed in on all sides and
    # hard to fill
    if num_hints == 5:
        ord2pos_ = [i for i in ord2pos_ if i != 33]
        ord2pos_.insert(ord2pos_.index(17), 33)
        
    # redo the order with just the hint pieces
    for pos1 in hints_dict:
        ord2pos += [pos1]
    # copy locations from the original order to the new order if they
    # aren't hints
    for pos1 in ord2pos_:
        if pos1 not in hints_dict:
            ord2pos += [pos1]
    # build pos2ord from ord2pos
    for ord1, pos1 in enumerate(ord2pos):
        pos2ord[pos1] = ord1

if sys.argv[1] == 'row':
    build_rowscan()
elif sys.argv[1] == 'spiral':
    build_spiral()
else:
    print('unknown piece order scheme: {sys.argv[1]}')
    sys.exit(0)

with open('genheader.h', 'w') as fp:
    pdata = "".join(piece_data.split("\n"))
    fp.write(f'char *piece_data = "aaaa/{pdata}";\n')
    pdata = ','.join(map(str, ord2pos))
    fp.write(f'int ord2pos[] = {{ {pdata} }};\n')
    pdata = ','.join(map(str, pos2ord))
    fp.write(f'int pos2ord[] = {{ {pdata} }};\n')
    fp.write(f'#define WIDTH 16\n')
    fp.write(f'#define HEIGHT 16\n')
    fp.write(f'#define NUM_HINTS {num_hints}\n')

with open('gensrc.c', 'w') as fp:
    src2 = ''
    # fp.write('ord1=0;\n');
    # fp.write('while (TRUE) {\n');
    # fp.write(' switch(ord1) {\n');
    for ord1 in range(width*height):
        pos1 = ord2pos[ord1]
        r1 = pos1 // width
        c1 = pos1 % width
        src2 = f'  case{ord1}:\n'
        # src2 += f'  for(ord1=0; ord1<width*height; ord1++) {\n'
        # src2 += f'    int row1, col1, row2, col2, dir;\n'
        src2 += f'    if (!Q[{ord1}].active) {{\n'
        # src2 += f'      pos1 = ord2pos[{ord1}];\n'
        k = ''
        for dir in range(4):
            # src2 += f'      for(dir=0; dir<4; dir++) {\n'
            # src2 += f'    k *= fit_size1;\n'
            r2 = r1 + dirs[dir][0]
            c2 = c1 + dirs[dir][1]
            if r2 < 0 or r2 >= width or c2 < 0 or c2 >= height:
                new_k = '0'
            else:
                ord2 = pos2ord[r2*width+c2]
                if ord2 >= ord1:
                    new_k = '23'
                else:
                    new_k = f'Q[{ord2}].pieces->piece->edges[{(dir+2)%4}]'
            k = f'({k})*24+{new_k}' if len(k) else new_k
        src2 += f'      Q[{ord1}].pieces = fit_table[{k}];\n'
        src2 += f'      Q[{ord1}].active = TRUE;\n'
        src2 += f'    }}\n'
        src2 += f'    if (Q[{ord1}].pieces->piece) {{\n'
        src2 += f'      placed[Q[{ord1}].pieces->piece->piecenum] -= 1;\n'
        src2 += f'    }}\n'
        src2 += f'  case{ord1}_dup:\n'
        src2 += f'    Q[{ord1}].pieces = Q[{ord1}].pieces->next;\n'
        src2 += f'    if (!Q[{ord1}].pieces) {{\n'
        src2 += f'      Q[{ord1}].active = FALSE;\n'
        if ord1 > 0:
            src2 += f'      goto case{ord1 - 1};\n'
        else:
            src2 += f'      return 0;\n'
        src2 += f'    }}\n'
        src2 += f'    p = Q[{ord1}].pieces->piece;\n'
        if ord1 < num_hints:
            src2 += f'    if (placed[p->piecenum] || p->piecenum != {hints_dict[pos1][0]} || p->rot != {hints_dict[pos1][1]}) {{\n'
        else:
            src2 += f'    if (placed[p->piecenum]) {{\n'
        # src2 += f'      placed[p->piecenum]++;\n'
        src2 += f'      goto case{ord1}_dup;\n'
        src2 += f'    }}\n'
        src2 += f'    nodes += 1;\n'
        src2 += f'    if (nodes % status_interval == 0) {{\n'
        src2 += f'      if (print_status(0))\n'
        src2 += f'        goto case0;\n'
        src2 += f'    }}\n'
        src2 += f'    placed[p->piecenum] = 1;\n'
        src2 += f'    if ({ord1+1} > best) {{\n'
        src2 += f'      best = {ord1+1};\n'
        src2 += f'      best_node = nodes;\n'
        # if ord1+1 >= 94:
        if ord1+1 >= 127:
            src2 += f'      print_puzz({ord1+1});\n'
        src2 += f'    }}\n'

        fp.write(src2)
