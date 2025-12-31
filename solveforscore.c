#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define WIDTH 16
#define HEIGHT 16
#define MAX_PIECES 256
#define MAX_FIT_KEYS 1000
#define MAX_FIT_PER_KEY 100
#define MAX_QUEUE 100000

typedef struct {
  int piecenum;
  int rot;
} Piece;

typedef struct {
  unsigned char vals[4];
} PieceData;

typedef struct {
  unsigned char up;
  unsigned char left;
  bool right_edge;
  bool bottom_edge;
} FitKey;

typedef struct {
  Piece pieces[MAX_FIT_PER_KEY];
  int count;
} FitList;

typedef struct {
  Piece solution[WIDTH * HEIGHT];
  bool remain[MAX_PIECES + 1];
  int skips;
  int score;
} QueueItem;

// Global variables
PieceData pieces[MAX_PIECES + 1][4];
int piece_count = 0;
FitKey fit_keys[MAX_FIT_KEYS];
FitList fit_lists[MAX_FIT_KEYS];
int fit_count = 0;
Piece pieceorder[MAX_PIECES * 4];
int pieceorder_count = 0;
int order[WIDTH * HEIGHT];
long long nodes = 0;
int highscore = 0;
int highdisp = 0;
char skipdisp[64];
time_t start_time;
int minpos = 256;
int maxpos = 0;
bool stuck = true;
int first208 = 0;
int hints_pos[10];
Piece hints_piece[10];
int hints_count = 0;
bool hintp[MAX_PIECES + 1];
int glob_skiplist[10];
int glob_skiplist_len = 0;

const char* piece_data = 
  "bdaa/beaa/cdaa/dcaa/gbab/hcab/jbab/jfab/mdab/oeab/pcab/teab/tfab/veab/hbac/idac/"
  "kfac/nfac/ocac/pcac/qeac/rbac/rfac/sbac/vbac/gcad/gdad/hdad/idad/ofad/pcad/sdad/"
  "tcad/tead/ufad/wead/ibae/lfae/mfae/ncae/ndae/pbae/pcae/pdae/qbae/seae/teae/ueae/"
  "gfaf/hbaf/hcaf/jbaf/oeaf/qeaf/qfaf/tdaf/ubaf/ufaf/vcaf/wdaf/jigg/kogg/hlgh/gtgi/"
  "iwgi/kkgi/mhgi/sjgi/wtgi/logl/orgl/kigm/pqgm/spgm/tlgm/kpgn/npgn/lugo/slgo/uvgo/"
  "nigp/iigq/mqgq/ingr/jkgr/trgr/usgr/gvgs/jwgs/qugs/mrgt/npgt/nqgt/qvgt/rkgt/vlgv/"
  "qngw/stgw/vjgw/wvgw/rphh/ruhh/unhh/wjhh/qphi/lthj/nthj/qphj/umhj/prhk/rphk/snhl/"
  "suhl/kqhm/orhm/rmhm/wuhm/lvhn/urho/twhp/kwhq/juhs/knhs/sphs/rwht/vpht/kjhu/nvhu/"
  "qlhu/sthu/jshv/rkhv/kuhw/mqhw/qlhw/ushw/soii/jlij/jmij/jrij/nvij/pjij/urij/vvij/"
  "lqik/iril/iwil/lkil/pril/jmin/qwin/mmio/mnio/wuio/ooiq/oriq/woiq/ouis/tjis/vvit/"
  "jsiu/ksiu/pmiw/quiw/lqjk/qtjk/lljm/mpjn/nvjn/qtjo/sujo/vmjo/ppjp/rsjp/ovjq/onjr/"
  "kqjs/rujt/tpjt/ouju/mujv/omjv/ntkk/wokl/nrkm/ttkm/vokn/kvko/lnko/unko/llkp/pskp/"
  "mokq/vmkr/vwkr/wpkr/mwks/ntks/ssks/nlkt/vtku/woku/rnkv/rtkv/ulkv/wvkv/woll/nwlm/"
  "tplm/mnlo/uplo/lulp/ttlq/lslr/qwlr/wvlr/npls/qrlu/mqlv/mulw/vvlw/vwlw/rwmm/somm/"
  "upmm/npmo/rrmo/ntmq/owms/utms/rsmt/rvmt/nsnn/uqno/oqnq/osnq/rpnq/qunr/pwns/vpns/"
  "ovnt/qvnt/prop/stop/wsoq/uwov/vwpp/rqpq/qvpr/uvps/wtqq/trqr/usrt/vusu/uwsw/vwtw";

void shuffle_array(int* arr, int n) {
  for (int i = n - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
  }
}

void shuffle_pieces(Piece* arr, int n) {
  for (int i = n - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    Piece temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
  }
}

int fit_key_hash(FitKey key) {
  return (key.up * 23 + key.left * 17 + key.right_edge * 3 + key.bottom_edge) % MAX_FIT_KEYS;
}

int find_fit_index(FitKey key) {
  int hash = fit_key_hash(key);
  for (int i = 0; i < fit_count; i++) {
    if (fit_keys[i].up == key.up && 
	fit_keys[i].left == key.left &&
	fit_keys[i].right_edge == key.right_edge &&
	fit_keys[i].bottom_edge == key.bottom_edge) {
      return i;
    }
  }
  return -1;
}

void add_to_fit(FitKey key, Piece piece) {
  int idx = find_fit_index(key);
  if (idx == -1) {
    idx = fit_count++;
    fit_keys[idx] = key;
    fit_lists[idx].count = 0;
  }
  fit_lists[idx].pieces[fit_lists[idx].count++] = piece;
}

void parse_pieces() {
  char data_copy[10000];
  strcpy(data_copy, piece_data);
    
  // Remove newlines
  int write_pos = 0;
  for (int i = 0; data_copy[i]; i++) {
    if (data_copy[i] != '\n') {
      data_copy[write_pos++] = data_copy[i];
    }
  }
  data_copy[write_pos] = '\0';
    
  int piecenum = 1;
  char* token = strtok(data_copy, "/");
    
  while (token != NULL && piecenum <= MAX_PIECES) {
    unsigned char t1[4];
    for (int i = 0; i < 4; i++) {
      t1[i] = token[i] - 'a';
    }
        
    for (int rot = 0; rot < 4; rot++) {
      for (int i = 0; i < 4; i++) {
	pieces[piecenum][rot].vals[i] = t1[(4 - rot + i) % 4];
      }
    }
        
    piecenum++;
    token = strtok(NULL, "/");
  }
  piece_count = piecenum - 1;
}

void build_fit_table() {
  for (int pn = 1; pn <= piece_count; pn++) {
    for (int rot = 0; rot < 4; rot++) {
      FitKey key;
      key.up = pieces[pn][rot].vals[0];
      key.left = pieces[pn][rot].vals[3];
      key.right_edge = (pieces[pn][rot].vals[1] == 0);
      key.bottom_edge = (pieces[pn][rot].vals[2] == 0);
            
      Piece p = {pn, rot};
      add_to_fit(key, p);
    }
  }
}

void init_solver(int seed) {
  srand(seed);
  start_time = time(NULL);
  nodes = 0;
  highscore = 0;
  highdisp = 0;
  skipdisp[0] = 0;
  stuck = true;
  first208 = 0;
    
  for (int i = 0; i < fit_count; i++) {
    shuffle_pieces(fit_lists[i].pieces, fit_lists[i].count);
  }
    
  pieceorder_count = 0;
  for (int pn = 1; pn <= piece_count; pn++) {
    for (int rot = 0; rot < 4; rot++) {
      pieceorder[pieceorder_count].piecenum = pn;
      pieceorder[pieceorder_count].rot = rot;
      pieceorder_count++;
    }
  }
  shuffle_pieces(pieceorder, pieceorder_count);
    
  // Initialize order (zigzag pattern)
  int idx = 0;
  for (int col = 0; col < 13; col++) {
    for (int row = 0; row < 16; row++) {
      order[idx++] = row * WIDTH + col;
    }
  }
  for (int row = 0; row < 16; row++) {
    for (int col = 13; col < 16; col++) {
      order[idx++] = row * WIDTH + col;
    }
  }
    
  // Initialize hints
  hints_count = 1;
  hints_pos[0] = 135;
  hints_piece[0].piecenum = 139;
  hints_piece[0].rot = 2;
    
  memset(hintp, 0, sizeof(hintp));
  for (int i = 0; i < hints_count; i++) {
    hintp[hints_piece[i].piecenum] = true;
  }
}

char* nodefmt(long long x, char* buf) {
  if (x < 1000) {
    sprintf(buf, "%lld", x);
    return buf;
  }
    
  double val = x;
  int mag = 0;
  while (val >= 1000) {
    mag++;
    val /= 1000;
  }
  sprintf(buf, "%.2f%c", val, " kmgtpezyb"[mag]);
  return buf;
}

void print_solution(QueueItem *current) {
  Piece* solution = current->solution;
  int count = current->score / 1000;
  int score = current->score;
  printf("best:", count);
  for (int row = 0; row < HEIGHT; row++) {
    for (int k = 0; k < WIDTH; k++) {
      Piece p = solution[row * WIDTH + k];
      if (p.piecenum > 0) {
	printf(" %d/%d", p.piecenum, p.rot);
      } else {
	printf(" 0/0");
      }
    }
  }
  printf("\n");
    
  char url[1200];
  int skipsum = 0;
  for (int i = 0; i < glob_skiplist_len; i++) {
    skipsum += glob_skiplist[i];
  }
    
#if 0
  strcpy(url, "https://e2.bucas.name/#puzzle=EternityII&board_w=16&board_h=16&board_edges=");
  int urllen = strlen(url);
  for (int row = 0; row < HEIGHT; row++) {
    for (int k = 0; k < WIDTH; k++) {
      Piece p = solution[row * WIDTH + k];
      for(int i=0; i<4; i++) {
	if (p.piecenum > 0) {
	  url[urllen++] = pieces[p.piecenum][p.rot].vals[i]+'a';
	} else {
	  url[urllen++] = 'a';
	}
	url[urllen] = 0;
      }
    }
  }
  puts(url);
  printf("\n");
#endif
  fflush(stdout);
}

void print_solution2(QueueItem *current) {
  Piece* solution = current->solution;
  int count = current->score / 1000;
  int score = current->score;
  printf("\n%d:\n", count);
  for (int row = 0; row < HEIGHT; row++) {
    for (int k = 0; k < WIDTH; k++) {
      Piece p = solution[row * WIDTH + k];
      if (p.piecenum > 0) {
	for(int i=0; i<4; i++) {
	  printf("%c", pieces[p.piecenum][p.rot].vals[i]+'a');
	}
	printf(" ");
	// printf("%02x ", p.piecenum - 1);
      } else {
	printf(".... ");
      }
    }
    printf("\n");
  }
  printf("\n");
    
  char url[1200];
  int skipsum = 0;
  for (int i = 0; i < glob_skiplist_len; i++) {
    skipsum += glob_skiplist[i];
  }
    
#if 1
  strcpy(url, "https://e2.bucas.name/#puzzle=EternityII&board_w=16&board_h=16&board_edges=");
  int urllen = strlen(url);
  for (int row = 0; row < HEIGHT; row++) {
    for (int k = 0; k < WIDTH; k++) {
      Piece p = solution[row * WIDTH + k];
      for(int i=0; i<4; i++) {
	if (p.piecenum > 0) {
	  url[urllen++] = pieces[p.piecenum][p.rot].vals[i]+'a';
	} else {
	  url[urllen++] = 'a';
	}
	url[urllen] = 0;
      }
    }
  }
  puts(url);
#endif
  printf("\n");
  
  fflush(stdout);
}

bool search1(Piece*, bool*, int, int, int *);

bool search2(Piece* start_layout, bool* start_remain, int goal_pos, 
             int skip_allow2, int *orig_skips, int orig_score, int s1_depth) {
  QueueItem* Q = malloc(sizeof(QueueItem) * MAX_QUEUE);
  int Q_head = 0, Q_tail = 1;
    
  // printf("skip_allow2 = %d\n", skip_allow2);
  memcpy(Q[0].solution, start_layout, sizeof(Piece) * WIDTH * HEIGHT);
  memcpy(Q[0].remain, start_remain, sizeof(bool) * (MAX_PIECES + 1));
  Q[0].skips = 0;
  Q[0].score = orig_score;
    
  bool found = false;
    
  while (Q_head < Q_tail) {
    // QueueItem current = Q[Q_head++];
    QueueItem current = Q[--Q_tail];
    nodes++;
        
    if (nodes > 500000 && stuck) {
      printf("restarting\n");
      free(Q);
      return false;
    }
        
    if (nodes % 16000000 == 0) {
      char buf[50];
      int t = time(NULL) - start_time;
      if (t > 0) {
	// status best=207 (119m) nodes=200000000 time=5 rate=40000000 restarts=1 last=0
	printf("status best=%d nodes=%lld range=%d-%d rate=%lld skips=%s time=%d\n", 
	       highdisp, nodes, minpos, maxpos, nodes / t, skipdisp, t);
	fflush(stdout);
      }
      minpos = 256;
      maxpos = 0;
    }
        
    int count = current.score / 1000;
        
    if (count >= 160) stuck = false;
    if (count == 208 && first208 == 0) first208 = nodes;
        
    if (current.score > highdisp) {
      highdisp = current.score;
      if (current.score > highscore) {
	highscore = current.score;
      }
      print_solution(&current);
      char buf1[50], buf2[50];
      skipdisp[0] = 0;
      for (int i=0; i<s1_depth; i++) {
	sprintf(skipdisp+strlen(skipdisp), "%d/", orig_skips[i]);
      }
      sprintf(skipdisp+strlen(skipdisp), "%d", current.skips);
      printf("nodes = %s, score = %d, first208 = %s, skips = %s\n", 
	     nodefmt(nodes, buf1), current.score, nodefmt(first208, buf2),
	     skipdisp);
      fflush(stdout);
    }
        
    if (count >= goal_pos) {
      // printf("reached the next goal\n");
      orig_skips[s1_depth] = current.skips;
      if (search1(current.solution, current.remain, current.score, s1_depth+1, orig_skips)) {
	found = true;
      }
      continue;
    }
        
    int pos = order[count];
    int newscore = 2;
    unsigned char up, left;
        
    if (pos < WIDTH) {
      up = 0;
      newscore--;
    } else {
      Piece prev = current.solution[pos - WIDTH];
      up = pieces[prev.piecenum][prev.rot].vals[2];
    }
        
    if (pos % WIDTH == 0) {
      left = 0;
      newscore--;
    } else {
      Piece prev = current.solution[pos - 1];
      left = pieces[prev.piecenum][prev.rot].vals[1];
    }
        
    FitKey key = {up, left, (pos % WIDTH == WIDTH - 1), (pos / WIDTH == HEIGHT - 1)};
    int fit_idx = find_fit_index(key);
        
    Piece* pool;
    int pool_size;
        
    // printf("current.skips = %d, skip_allow2 = %d\n", current.skips, skip_allow2);
    if (current.skips < skip_allow2) {
      pool = pieceorder;
      pool_size = pieceorder_count;
      // printf("trying to skip\n");
    } else {
      // printf("not trying to skip\n");
      if (fit_idx == -1) {
	// printf("no fits at line %d\n", __LINE__);
	continue;
      }
      pool = fit_lists[fit_idx].pieces;
      pool_size = fit_lists[fit_idx].count;
    }
        
    for (int pi = 0; pi < pool_size; pi++) {
      Piece p = pool[pi];
      /* char edgebuf[5]; */
      /* for(int ii=0; ii<4; ii++) { */
      /* 	edgebuf[ii] = pieces[p.piecenum][p.rot].vals[ii] + 'a'; */
      /* } */
      /* edgebuf[4] = 0; */
      /* printf("trying %s\n", edgebuf); */
      if (!current.remain[p.piecenum]) {
	// printf("skip at line %d\n", __LINE__);
	continue;
      }
            
      // Check hints
      bool hint_match = true;
      for (int h = 0; h < hints_count; h++) {
	if (hints_pos[h] == pos && 
	    (hints_piece[h].piecenum != p.piecenum || hints_piece[h].rot != p.rot)) {
	  hint_match = false;
	  // printf("hint conflict!\n");
	  break;
	}
	if (pos != hints_pos[h] && hintp[p.piecenum]) {
	  hint_match = false;
	  // printf("hint conflict!\n");
	  break;
	}
      }
      if (!hint_match) {
	// printf("skip at line %d\n", __LINE__);
	continue;
      }
            
      int newskips = 0;
      if (current.skips < skip_allow2) {
	// printf("try skipping\n");
	PieceData pc = pieces[p.piecenum][p.rot];
	if (pc.vals[0] != up) newskips++;
	if (pc.vals[3] != left) newskips++;
	if (newskips + current.skips > skip_allow2) {
	  // printf("skip at line %d\n", __LINE__);
	  continue;
	}
                
	if (((pos % WIDTH == 0) != (pc.vals[3] == 0)) ||
	    ((pos / WIDTH == 0) != (pc.vals[0] == 0)) ||
	    ((pos % WIDTH == WIDTH - 1) != (pc.vals[1] == 0)) ||
	    ((pos / WIDTH == HEIGHT - 1) != (pc.vals[2] == 0))) {
	  continue;
	}
      }
            
      if (Q_tail >= MAX_QUEUE) {
	continue;
      }
            
      // printf("adding position to Q. size = %d\n", Q_tail - Q_head);
      memcpy(Q[Q_tail].solution, current.solution, sizeof(Piece) * WIDTH * HEIGHT);
      memcpy(Q[Q_tail].remain, current.remain, sizeof(bool) * (MAX_PIECES + 1));
      Q[Q_tail].solution[pos] = p;
      Q[Q_tail].remain[p.piecenum] = false;
      Q[Q_tail].skips = current.skips + newskips;
      Q[Q_tail].score = current.score + 1000 + newscore - newskips;
      Q_tail++;
            
      if (count < minpos) minpos = count;
      if (count > maxpos) maxpos = count;
    }
  }
    
  free(Q);
  return found;
}

bool search1(Piece* start_layout, bool* start_remain, int orig_score, int depth, int *prev_skips) {
  int ranges[] = { 192,16,15,12, 9, 6, 6 };
  int limits[] = {   0, 1, 3, 3, 4, 4, 12 };
  // int limits[] = {   0, 1, 3, 4, 4, 4, 10 };
  int range=0;
  if (depth > 6) {
    return true;
  }
  
  for (int i = 0; i<=depth; i++) {
    range += ranges[i];
  }
  return search2(start_layout, start_remain, range, limits[depth], prev_skips, orig_score, depth);
}

int main(int argc, char *argv[]) {
  parse_pieces();
  build_fit_table();
  int skips[7];
    
  while (true) {
    init_solver(atoi(argv[2]));
        
    Piece start_layout[WIDTH * HEIGHT];
    bool start_remain[MAX_PIECES + 1];
    memset(start_layout, 0, sizeof(start_layout));
    for (int i = 0; i <= MAX_PIECES; i++) {
      start_remain[i] = (i > 0 && i <= piece_count);
    }
        
    glob_skiplist_len = 0;
    search1(start_layout, start_remain, 0, 0, skips);
  }
    
  return 0;
}
