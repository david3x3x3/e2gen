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

int height=WIDTH, width=HEIGHT, best=0, core=0, status_interval, restarts=0;
int fit_size1, fit_size2, placed[WIDTH*HEIGHT+1];
long long nodes=0, best_node=0;
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

piecelist_t **fit_table;

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
bignum_fmt(char *s, long long val) {
  char unit_names[] = "!kmgtpezy", unit[2];
  int unit_num=0;

  while (val >= 10000) {
    unit_num++;
    val /= 1000;
  }
  if (val < 1000) {
    sprintf(unit, "%c", unit_names[unit_num]);
    if (unit[0] == '!') unit[0] = '\0';
    sprintf(s, "%lld%s", val, unit);
  } else {
    unit_num++;
    sprintf(unit, "%c", unit_names[unit_num]);
    if (unit[0] == '!') unit[0] = '\0';
    sprintf(s, "%lld.%02lld%s", val/1000, (val%1000)/10, unit);
  }
}

int
restart() {
  piecelist_t *pl;
  piece_t *p;
  int i, j, k, rot, ord1, pos1, piecenum;
  int porder[WIDTH*HEIGHT];

  /* if ((++restarts % 100) == 0) { */
  /*   printf("restart #%d\n", restarts); */
  /* } */
  restarts++;
  nodes=best_node=0;
  best=0;
  status_interval=60000;
  for(i=0; i<=width*height; i++) {
    placed[i] = (i==0);
  }

  for(i=0; i<fit_size2; i++) {
    while(fit_table[i]) {
      pl = fit_table[i];
      fit_table[i] = pl->next;
      free(pl);
    }
  }
  // recompute the fit table with a random order
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

  start_time = time(NULL)-1;
  bzero(Q, width*height*sizeof(square_t));
  return 1;
}

int
print_status(int after_best) {
  char msg[1024];
  char msg2[1024];
  long long rate;
  char nodes_disp[6], rate_disp[6], bestn_disp[6];
  
  bignum_fmt(nodes_disp, nodes);
  rate = nodes/(time(NULL)-start_time);
  bignum_fmt(rate_disp, rate);
  bignum_fmt(bestn_disp, best_node);
  sprintf(msg, "best=%d (%s) nodes=%s time=%lld rate=%s restarts=%d", best,
	  bestn_disp, nodes_disp, time(NULL)-start_time, rate_disp, restarts);
  if (after_best || nodes >= 1000000) {
#ifdef EMSCRIPTEN
    sprintf(msg2, "postMessage({msgType:'status',data:'%s','core':%d});", msg, core);
    emscripten_run_script(msg2);
#else
    printf("%s\n", msg);
    fflush(stdout);
#endif
  }
  if (nodes >= 60000) {
    if (!after_best)
      if (best<107 ||
	  (nodes >= 1000000 && best<117) ||
	  (nodes >= 1000000000 && best<192) ||
	  (nodes >= 20000000000 && best<208)) {
	// these thresholds were designed for spiral with hints, but
	// they work ok for other arrangements
	return restart();
      }
    status_interval = 100000000;
  }
  return 0;
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
  print_status(1);
}

int
origmain(char *argv1, char *argv2) {
  int row, col, i, j, k, piecenum, rot, k1, k2, k3, k4, ord1, ord2,
    pos1, pos2;
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
  fit_size1 = max_edge-'a'+2;
  fit_size2 = 1;
  for (i=0; i<4; i++) {
    fit_size2 *= fit_size1;
  }
  fit_table = calloc(fit_size2, sizeof(piecelist_t *));
  int dirs[4][2] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};
  Q = calloc(width*height, sizeof(square_t));
  restart();
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
