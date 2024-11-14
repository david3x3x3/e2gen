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
int fit_size1, fit_size2, placed[WIDTH*HEIGHT+1], bestbest=0, *fit_entries=NULL;
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

piecelist_t *
shuffle_piecelist(piecelist_t *pl_in) {
  int int_order[WIDTH*HEIGHT*4], j, i;
  piecelist_t *pl_order[WIDTH*HEIGHT*4], *pl;
  
  j=0;
  for(pl=pl_in; pl!=NULL; pl=pl->next) {
    //printf("%s ", pl->piece->edgestr);
    int_order[j]=j;
    pl_order[j] = pl;
    j++;
  }
  shuffle(int_order, j);
  for(i=0; i<j-1; i++) {
    pl_order[int_order[i]]->next = pl_order[int_order[i+1]];
  }
  pl_order[int_order[j-1]]->next = NULL;
  return pl_order[int_order[0]];
}

void
bignum_fmt(char *s, long long val) {
  char unit_names[] = "!kmgtpezy", unit[2];
  int unit_num=0;

  while (val >= 100000) {
    unit_num++;
    val /= 1000;
  }
  if (val >= 1000) {
    unit_num++;
  }
  sprintf(unit, "%c", unit_names[unit_num]);
  if (unit[0] == '!') unit[0] = '\0';
  if (val < 1000) {
    // NNN
    sprintf(s, "%lld%s", val, unit);
  } else if (val < 10000) {
    // N.NN
    sprintf(s, "%lld.%02lld%s", val/1000, (val%1000)/10, unit);
  } else {
    // NN.N
    sprintf(s, "%lld.%lld%s", val/1000, (val%1000)/100, unit);
  }
}

int
restart() {
  piecelist_t *pl;
  piece_t *p;
  int i, j, k, m, rot, ord1, pos1, piecenum;
  int porder[WIDTH*HEIGHT];
  char msg[1024];
  char msg2[1024];

  if (best > bestbest) {
    bestbest = best;
  }
  restarts++;
  if ((restarts % 5000) == 0) {
    sprintf(msg, "best=%d restarts=%d", bestbest, restarts);
#ifdef EMSCRIPTEN
    sprintf(msg2, "postMessage({msgType:'status',data:'%s','core':%d});", msg, core);
    emscripten_run_script(msg2);
#else
    printf("status %s\n", msg);
    fflush(stdout);
#endif
  }
  nodes=best_node=0;
  best=0;
  status_interval=20000;
  for(i=0; i<=width*height; i++) {
    placed[i] = (i==0);
  }
  start_time = time(NULL)-1;
  bzero(Q, width*height*sizeof(square_t));
  if (fit_entries != NULL) {
    // we built the fit_table once, so just shuffle instead of rebuilding
    for(i=0; fit_entries[i] >= 0; i++) {
      pl = fit_table[fit_entries[i]];
      pl->next = shuffle_piecelist(pl->next);
    }
    return 1;
  }
  // randomize the initial building of the fit table
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

  // figure out which entries in fit_table are populated to make it
  // easier to go back and scramble them.
  printf("fit_size2 = %d\n", fit_size2);
  i = m=0;
  for (k=0; k<fit_size2; k++) {
    if(fit_table[k] != NULL) {
      m++;
      //printf("fit_table[%d]: ", k);
      j=0;
      for(pl=fit_table[k]; pl!=NULL; pl=pl->next) {
	//printf("%s ", pl->piece->edgestr);
	j++;
      }
      //printf(" (len=%d)\n", j);
      if(j>1) {
	// count how many entries there are
	i++;
      }
    }
  }
  printf("%d entries\n", m);
  printf("%d long entries\n", i);
  fit_entries = malloc((i+1)*sizeof(int));
  i = 0;
  for (k=0; k<fit_size2; k++) {
    if(fit_table[k] != NULL) {
      j=0;
      for(pl=fit_table[k]; pl!=NULL; pl=pl->next) {
	//printf("%s ", pl->piece->edgestr);
	j++;
      }
      if(j>1) {
	// record the entries
	fit_entries[i++] = k;
      }
    }
  }
  fit_entries[i++] = -1; // so we can find the end
  
  // add a null tile to the beginning of each list to simplify the search logic
  for (k=0; k<fit_size2; k++) {
    pl = malloc(sizeof(piecelist_t));
    pl->piece = NULL;
    pl->next = fit_table[k];
    fit_table[k] = pl;
  }
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
  sprintf(msg, "best=%d (%s) nodes=%lld time=%lld rate=%lld restarts=%d", best,
	  bestn_disp, nodes, (long long)time(NULL)-start_time, rate,
	  restarts);
  if (after_best || nodes >= 1000000) {
#ifdef EMSCRIPTEN
    sprintf(msg2, "postMessage({msgType:'status',data:'%s','core':%d});", msg, core);
    emscripten_run_script(msg2);
#else
    printf("status %s\n", msg);
    fflush(stdout);
#endif
  }
  if (nodes >= 20000) {
    status_interval = 100000000;
  }
  if (!after_best) {
    if (best<127 ||
	(nodes >= 100000000 && best<192) ||
	(nodes >= 20000000000 && best<208)) {
      // these thresholds were designed for spiral with hints, but
      // they work ok for other arrangements
      return restart();
    }
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
  printf("seed = %d\n", atoi(argv2));
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
    //printf("%d %s ", piecenum, temp_edges);
    tempstr[4] = '\0';
    for(i=0; i<4; i++) {
      strncpy(tempstr, temp_edges+4-i, 4);
      pieces[piecenum][i].piecenum = piecenum;
      pieces[piecenum][i].rot = i;
      for (j=0; j<4; j++) {
	pieces[piecenum][i].edges[j] = tempstr[j] - 'a';
      }
      strcpy(pieces[piecenum][i].edgestr, tempstr);
      //printf("%s ", pieces[piecenum][i].edgestr);
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
