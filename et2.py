import random
import sys

puzname = sys.argv[1]
if puzname == 'eternity2':
    width, height = 16, 16
else:
    width, height = map(int, puzname.split('x'))

print(f'width = {width}')

with open('puzzles.txt') as fp:
    it = iter([l.strip() for l in fp.readlines()])
    puzzles = dict(zip(it, it))

pieces0 = ['aaaa'] + puzzles[puzname].split('/')

pieces = dict()
for i, p in enumerate(pieces0):
    p2 = []
    for rot in range(4):
        pieces[(i, rot)] = p
        p = p[3] + p[:3]
fit = dict()
for key1, val in pieces.items():
    key2 = (val[0], val[3], val[1] == 'a', val[2] == 'a')
    if not fit.get(key2):
        fit[key2] = [key1]
    else:
        fit[key2].insert(0, key1)
print(len(pieces0), pieces0)
# for k in fit:
#     random.shuffle(fit[k])
Q = [([(0, 0)]*width, set(range(1,len(pieces0))))]
solcount = 0
nodes = 0
best = 0
while Q:
    solution, remain = Q.pop()
    nodes += 1
    if nodes % 1000000 == 0:
        print(f'nodes = {nodes}')
        # self.postMessage(to_js({'progress': nodes}, dict_converter=Object.fromEntries))
    if len(solution) > best:
        # print('\n', len(solution)-16, ' '.join([f'{p[0]}/{p[1]}' for p in solution[width:]]))
        url = ''.join([pieces[p] for p in solution[width:]])
        for r in remain:
            url += pieces[(r, 0)]
        url = 'https://e2.bucas.name/#puzzle=EternityII&board_w=16&board_h=16&board_edges=' + url
        best = len(solution)
        p2 = [f'{p[0]}/{p[1]}' for p in solution[width:]]
        # self.postMessage(to_js({'pieces': p2, 'url': url}, dict_converter=Object.fromEntries))
        # print({'pieces': p2, 'url': url})
    if len(solution) == width+int(sys.argv[2]):
        #if solcount % 100000 == 0:
        if True:
            disp = ' '.join([f'{p[0]}/{p[1]}' for p in solution[width:]])
            print(f'found #{solcount} - {disp}')
        solcount += 1
    else:
        for p in fit.get((pieces[solution[-width]][2],
                          pieces[solution[-1]][1],
                          len(solution) % width == width - 1,
                          len(solution) // width == height),[]):
            (piecenum, rot) = p
            if len(solution[width:]) == 119:
                if pieces[(piecenum, rot)][2] != pieces[(139, 2)][0]:
                    continue
            if len(solution[width:]) < 135:
                if piecenum == 139:
                    continue
            if len(solution[width:]) == 135:
                if piecenum != 139 or rot != 2:
                    continue
            if piecenum in remain:
                Q += [(solution + [p], remain - set([piecenum]))]
print(str(solcount) + ' solutions')
print(str(nodes) + ' nodes')
