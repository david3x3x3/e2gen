#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <time.h>
#include "genheader.h"

#define TRUE 1
#define FALSE 0

int height=WIDTH, width=HEIGHT, best=0, core=0;
long long nodes=0;
time_t start_time;

typedef struct piece_s {
  int piecenum, rot;
  int edges[4];
  char edgestr[5];
} piece_t;

piece_t pieces[WIDTH*HEIGHT+1][4];

typedef struct piecelist_s {
  piece_t *piece;
  struct piecelist_s *next;
} piecelist_t;

typedef struct {
  int active;
  piecelist_t *pieces;
} square_t;

square_t *Q;

void
shuffle(int *array, size_t n) {
  if (n > 1) {
    size_t i;
    for (i = 0; i < n - 1; i++) {
      size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
      int t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}

void
print_status() {
  char msg[1024];
  char msg2[1024];
  // sprintf(msg, "status {'best': %d, 'nodes': %lld, 'time': %lld, 'rate': %lld}\n", best, nodes, time(NULL)-start_time, nodes/(time(NULL)-start_time));
  sprintf(msg, "best=%d nodes=%lld time=%lld rate=%lld", best, nodes, time(NULL)-start_time, nodes/(time(NULL)-start_time));
#ifdef EMSCRIPTEN
  fflush(stdout);
  sprintf(msg2, "postMessage({msgType:'status',data:'%s','core':%d});", msg, core);
  emscripten_run_script(msg2);
#else
  printf("%s", msg);
#endif
}

#ifdef __EMSCRIPTEN__
char best_buf[8192];
#endif

void
print_puzz(int ord) {
  int pos1, ord1;
  piece_t *p;
#ifdef __EMSCRIPTEN__
  sprintf(best_buf, "postMessage({msgType:'best',data:[%d,[",ord);
  for(pos1=0; pos1<width*height; pos1++) {
    ord1 = pos2ord[pos1];
    if (ord1 < ord) {
      p = Q[ord1].pieces->piece;
      sprintf(best_buf+strlen(best_buf), "[%d,%d],", p->piecenum, p->rot);
      //printf("%s ", p->edgestr);
    } else {
      sprintf(best_buf+strlen(best_buf), "[%d,%d],", 0, 0);
    }
  }
  strcpy(best_buf+strlen(best_buf),"]]})");
  emscripten_run_script(best_buf);
#else
  printf("best: ");
  for(pos1=0; pos1<width*height; pos1++) {
    ord1 = pos2ord[pos1];
    if (ord1 < ord) {
      p = Q[ord1].pieces->piece;
      printf("%d/%d ", p->piecenum, p->rot);
      //printf("%s ", p->edgestr);
    } else {
      printf("%d/%d ", 0, 0);
    }
  }
  printf("\n");
#endif
  print_status();
}

int
origmain(char *argv1, char *argv2) {
  int row, col, i, j, k, piecenum, rot, k1, k2, k3, k4, ord1, ord2,
    placed[WIDTH*HEIGHT+1],pos1, pos2;
  char temp_edges[9], c, max_edge = 'a', tempstr[5], msg[128];
  piecelist_t *pl;
  piece_t *p;
  unsigned int rnd=0;

  core = atoi(argv1);
  sprintf(msg,"postMessage('core = %d');", core);
#ifdef __EMSCRIPTEN__
  emscripten_run_script(msg);
#else
  puts(msg);
#endif

#ifdef __EMSCRIPTEN__
  rnd = EM_ASM_INT({
      return Math.floor(Math.random() * 2**32);
    });
  sprintf(msg,"postMessage('seed = %u');", rnd);
  emscripten_run_script(msg);
  srand(rnd);
#else
  //srand(time(NULL)*1000+getpid()%1000);
  srand(atoi(argv2));
#endif
  for(piecenum=0; piecenum<=width*height; piecenum++) {
    placed[piecenum] = (piecenum==0);
    k = 0;
    for(j=0; j<2; j++) {
      for(i=0; i<4; i++) {
	c = piece_data[piecenum*5+i];
	if (c > max_edge) {
	  max_edge = c;
	}
	temp_edges[k++] = c;
      }
    }
    temp_edges[k] = '\0';
    //printf("%d %s\n", piecenum, temp_edges);
    tempstr[4] = '\0';
    for(i=0; i<4; i++) {
      strncpy(tempstr, temp_edges+4-i, 4);
      pieces[piecenum][i].piecenum = piecenum;
      pieces[piecenum][i].rot = i;
      for (j=0; j<4; j++) {
	pieces[piecenum][i].edges[j] = tempstr[j] - 'a';
      }
      strcpy(pieces[piecenum][i].edgestr, tempstr);
    }
    //printf("\n");
  }
  //printf("piece_data = %s\n", piece_data);
  printf("max_edge = %d\n", max_edge-'a');
  int fit_size1 = max_edge-'a'+2;
  int fit_size2 = 1;
  for (i=0; i<4; i++) {
    fit_size2 *= fit_size1;
  }
  piecelist_t **fit_table = calloc(fit_size2, sizeof(piecelist_t *));
  int porder[WIDTH*HEIGHT];
  for (ord1=0; ord1<width*height; ord1++) {
    porder[ord1] = ord1+1;
  }
  shuffle(porder, width*height);
  for(pos1=0; pos1<width*height; pos1++) {
    piecenum = porder[pos1];
    for (rot=0; rot<4; rot++) {
      p = &pieces[piecenum][rot];
      // printf("checking %d/%d\n", p->piecenum, p->rot);
      // printf("%d/%d\n", piecenum, rot);
      for (i=0; i<16; i++) {
	int ok = TRUE;
	k = 0;
	for (j=0; j<4; j++) {
	  k *= fit_size1;
	  if (i & 1<<j) {
	    //printf("yes ");
	    k += p->edges[j];
	  } else {
	    //printf("no  ");
	    if (p->edges[j] != 0) {
	      k += fit_size1-1;
	    } else {
	      ok = FALSE;
	      break;
	    }
	  }
	}
	if (ok) {
	  pl = malloc(sizeof(piecelist_t));
	  pl->piece = p;
	  pl->next = fit_table[k];
	  fit_table[k] = pl;
	}
	//printf("\n");
      }
    }
  }
  // add a null tile to the beginning of each list to simplify the search logic
  for (k=0; k<fit_size2; k++) {
    pl = malloc(sizeof(piecelist_t));
    pl->piece = NULL;
    pl->next = fit_table[k];
    fit_table[k] = pl;
  }
  if (FALSE) {
    for (i=0; i<fit_size2; i++) {
      k1 = i;
      k4 = k1 % fit_size1; k1 /= fit_size1;
      k3 = k1 % fit_size1; k1 /= fit_size1;
      k2 = k1 % fit_size1; k1 /= fit_size1;
      if (fit_table[i] != NULL) {
	printf("%d %d %d %d %d - ", i, k1, k2, k3, k4);
	for(pl = fit_table[i]; pl != NULL; pl = pl->next) {
	  printf("%s ", pl->piece->edgestr);
	}
	printf("\n");
      }
    }
  }
  printf("table size = %d\n", fit_size2);
  int dirs[4][2] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};
  Q = calloc(width*height, sizeof(square_t));
  start_time = time(NULL)-1;
#include "gensrc.c"
}

#ifdef __EMSCRIPTEN__

int
main() {
  emscripten_run_script(
    "onmessage = function(e) {"
    " console.log('Message received from main script: ' + e.data);"
    "Module.ccall('origmain','number',['string','string'],['0',e.data]);}");
  emscripten_run_script("postMessage('worker is ready');");
  printf("and we're off...\n");
  return 0;
}

#else

int
main(int argc, char *argv[]) {
  origmain(argv[1],argv[2]);
  exit(0);
}
#endif
