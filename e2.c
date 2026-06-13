#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

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
int solutions = 0;
int rownum = -1;
int initial_rownum = -1;
char puzzle_name[64] = "";

/* save/restore state */
int current_ord = 0;
int restore_ord = -1;
char save_filename[256] = "";
time_t last_save_time = 0;
#define SAVE_INTERVAL 300  /* seconds between periodic saves */

typedef struct { int piecenum, rot; } saved_piece_t;
saved_piece_t restore_pieces[WIDTH*HEIGHT];

static volatile int got_signal = 0;

void signal_handler(int sig) { (void)sig; got_signal = 1; status_interval = 1; }

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

/* Recompute the fit_table key for position ord given already-placed neighbors.
   Mirrors the per-position key expression emitted by spiral-gen.py. */
int compute_key(int ord) {
  int pos = ord2pos[ord];
  int r = pos / width;
  int c = pos % width;
  int dirs[4][2] = {{-1,0},{0,1},{1,0},{0,-1}};
  int k = 0;
  for (int dir = 0; dir < 4; dir++) {
    int r2 = r + dirs[dir][0];
    int c2 = c + dirs[dir][1];
    int new_k;
    if (r2 < 0 || r2 >= height || c2 < 0 || c2 >= width) {
      new_k = 0;
    } else {
      int ord2 = pos2ord[r2*width+c2];
      if (ord2 >= ord) {
        new_k = fit_size1 - 1;  /* unknown neighbor -> max_edge+1 */
      } else {
        new_k = Q[ord2].pieces->piece->edges[(dir+2)%4];
      }
    }
    k = k * fit_size1 + new_k;
  }
  return k;
}

/* Walk Q[0..target_ord-1] into the correct piecelist entries to match saved state. */
void restore_to_state(int target_ord) {
  int ord;
  for (ord = 0; ord < target_ord; ord++) {
    int key = compute_key(ord);
    piecelist_t *pl = fit_table[key];  /* sentinel head */
    while (pl->next != NULL) {
      pl = pl->next;
      if (pl->piece != NULL &&
          pl->piece->piecenum == restore_pieces[ord].piecenum &&
          pl->piece->rot    == restore_pieces[ord].rot) {
        break;
      }
    }
    Q[ord].pieces = pl;
    Q[ord].active = TRUE;
    placed[pl->piece->piecenum] = 1;
  }
  printf("restored to ord=%d\n", target_ord);
  fflush(stdout);
}

void save_state(char *filename, int ord) {
  int i;
  FILE *fp = fopen(filename, "w");
  if (!fp) { perror("save_state fopen"); return; }
  fprintf(fp, "ord=%d\n", ord);
  fprintf(fp, "rownum=%d\n", initial_rownum);
  fprintf(fp, "nodes=%lld\n", nodes);
  fprintf(fp, "best=%d\n", best);
  fprintf(fp, "best_node=%lld\n", best_node);
  fprintf(fp, "restarts=%d\n", restarts);
  fprintf(fp, "bestbest=%d\n", bestbest);
  fprintf(fp, "solutions=%d\n", solutions);
  fprintf(fp, "elapsed=%lld\n", (long long)(time(NULL) - start_time));
  for (i = 0; i < ord; i++) {
    fprintf(fp, "%d %d\n",
            Q[i].pieces->piece->piecenum,
            Q[i].pieces->piece->rot);
  }
  fclose(fp);
  printf("state saved to %s (ord=%d)\n", filename, ord);
  fflush(stdout);
}

int load_state(char *filename) {
  int ord, pn, rot, b, r, bb, sol, rn;
  long long n, bn, elapsed;
  FILE *fp = fopen(filename, "r");
  if (!fp) return 0;
  if (fscanf(fp, "ord=%d\n",         &ord)     != 1) goto bad;
  if (fscanf(fp, "rownum=%d\n",      &rn)      != 1) goto bad;
  if (fscanf(fp, "nodes=%lld\n",     &n)       != 1) goto bad;
  if (fscanf(fp, "best=%d\n",        &b)       != 1) goto bad;
  if (fscanf(fp, "best_node=%lld\n", &bn)      != 1) goto bad;
  if (fscanf(fp, "restarts=%d\n",    &r)       != 1) goto bad;
  if (fscanf(fp, "bestbest=%d\n",    &bb)      != 1) goto bad;
  if (fscanf(fp, "solutions=%d\n",   &sol)     != 1) goto bad;
  if (fscanf(fp, "elapsed=%lld\n",   &elapsed) != 1) goto bad;
  if (ord < 0 || ord > WIDTH*HEIGHT) goto bad;
  for (int i = 0; i < ord; i++) {
    if (fscanf(fp, "%d %d\n", &pn, &rot) != 2) goto bad;
    restore_pieces[i].piecenum = pn;
    restore_pieces[i].rot      = rot;
  }
  fclose(fp);
  rownum         = rn;
  initial_rownum = rn;
  nodes      = n;
  best       = b;
  best_node  = bn;
  restarts   = r;
  bestbest   = bb;
  solutions  = sol;
  start_time = time(NULL) - (time_t)elapsed;
  restore_ord = ord;
  printf("loaded state from %s (ord=%d, nodes=%lld, best=%d, elapsed=%llds)\n",
         filename, ord, n, b, elapsed);
  fflush(stdout);
  return 1;
bad:
  fclose(fp);
  printf("warning: could not parse save file %s, starting fresh\n", filename);
  fflush(stdout);
  return 0;
}

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
edge_c2i(char c) {
  char *alpha = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKL";
  return (int)(strchr(alpha, c) - alpha);
}

int
restart() {
  piecelist_t *pl, *pl2, *pl3;
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
  status_interval=200000;
  for(i=0; i<=width*height; i++) {
    placed[i] = (i==0);
  }
  start_time = time(NULL)-1;
  bzero(Q, width*height*sizeof(square_t));
  if (fit_entries != NULL) {
    // we built the fit_table once, so just shuffle instead of rebuilding
    for(i=0; fit_entries[i] >= 0; i++) {
      pl = fit_table[fit_entries[i]];
      // pl->next = shuffle_piecelist(pl->next);
    }
    return 1;
  }
  // randomize the initial building of the fit table
  for (ord1=0; ord1<width*height; ord1++) {
    porder[ord1] = ord1+1;
  }
  // shuffle(porder, width*height);
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
  printf("fit_size1 = %d\n", fit_size1);
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

  // reverse the fit entries so that we search in a normal order
  for(i=0; fit_entries[i] >= 0; i++) {
    pl3 = NULL;
    pl = fit_table[fit_entries[i]];
    while(pl->next) {
      pl2 = pl->next;
      pl->next = pl->next->next;
      pl2->next = pl3;
      pl3 = pl2;
    }
    pl->next = pl3;
  }

  return 1;
}

int
print_status(int after_best, int last) {
  char msg[1024];
  char msg2[1024];
  long long rate;
  char nodes_disp[6], rate_disp[6], bestn_disp[6];

  bignum_fmt(nodes_disp, nodes);
  rate = nodes/(time(NULL)-start_time);
  bignum_fmt(rate_disp, rate);
  bignum_fmt(bestn_disp, best_node);
  sprintf(msg, "best=%d (%s) nodes=%lld time=%lld rate=%lld restarts=%d last=%d", best,
	  bestn_disp, nodes, (long long)time(NULL)-start_time, rate,
	  restarts, last);
  if (last || after_best || nodes >= 1000000) {
#ifdef EMSCRIPTEN
    sprintf(msg2, "postMessage({msgType:'status',data:'%s','core':%d});", msg, core);
    emscripten_run_script(msg2);
#else
    printf("status %s\n", msg);
    fflush(stdout);
#endif
  }
  if (nodes >= 200000) {
    status_interval = 1000000000;
  }
  /* periodic save and signal check (only at the nodes-interval checkpoint,
     not when called after placing a best piece, so current_ord is correct) */
  if (!after_best && save_filename[0]) {
    time_t now = time(NULL);
    if (got_signal || now - last_save_time >= SAVE_INTERVAL) {
      save_state(save_filename, current_ord);
      last_save_time = now;
    }
    if (got_signal) {
      printf("interrupted, exiting\n");
      fflush(stdout);
      exit(0);
    }
  }
  if (!after_best) {
    if (width == 16 && height == 16 && (
	best<127 ||
	//	(nodes >= 100000000 && best<192) ||
	// (nodes >= 20000000000 && best<208))) {
	(nodes >= 1000000000 && best<160) ||
	(nodes >= 20000000000 && best<176))) {
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

// heuristic for first row of 10x10

// counts the number of duplicated edge types between pieces in first row to make sure there are more than average

int
edge_check(int ord) {
  int pos1, ord1, edges[4], i, counts[7];
  piece_t *p;
  for(i=0; i<4; i++) {
    edges[i] = 0;
  }
  for(i=0; i<7; i++) {
    counts[i] = 0;
  }
  for(pos1=0; pos1<ord; pos1++) {
    p = Q[pos2ord[pos1]].pieces->piece;
    edges[p->edges[1]-1]++;
  }
  for(i=0; i<4; i++) {
    counts[edges[i]]++;
  }
  return counts[6] || (counts[5] && counts[3]);
}

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
  print_status(1, 0);
  if (ord == width*height) {
    solutions++;
  }
}

static int
read_password(char *buf, size_t len) {
  FILE *fp = fopen("password.txt", "r");
  if (!fp) { fprintf(stderr, "error: cannot open password.txt\n"); return 0; }
  int ok = (fgets(buf, len, fp) != NULL);
  fclose(fp);
  if (ok) buf[strcspn(buf, "\r\n")] = '\0';
  return ok;
}

static int
fetch_rownum_from_web(void) {
  char password[256];
  if (!read_password(password, sizeof(password))) return -1;

  char cmd[512];
  snprintf(cmd, sizeof(cmd),
    "wget -q -O - --auth-no-challenge --http-user=reporter '--http-password=%s'"
    " 'https://puzzlingaddiction.com/e2db/request_row'",
    password);
  FILE *fp = popen(cmd, "r");
  char response[1024] = {0};
  int ok = 0;
  if (fp) {
    size_t n = fread(response, 1, sizeof(response) - 1, fp);
    response[n] = '\0';
    ok = (pclose(fp) == 0);
  }

  if (!ok || response[0] == '\0') {
    fprintf(stderr, "error: wget failed or empty response: %s\n", response);
    return -1;
  }
  char *p = strstr(response, "\"row_num\"");
  if (!p) { fprintf(stderr, "error: no row_num in web response: %s\n", response); return -1; }
  p += strlen("\"row_num\"");
  while (*p == ' ' || *p == ':') p++;
  int rn = atoi(p);
  printf("rownum=%d (assigned by web service)\n", rn);
  fflush(stdout);
  return rn;
}

static void
report_result(void) {
  char password[256];
  if (!read_password(password, sizeof(password))) {
    fprintf(stderr, "warning: skipping result report (no password)\n");
    return;
  }

  char reporter[128] = "";
  FILE *rfp = fopen("reporter.txt", "r");
  if (rfp) {
    if (!fgets(reporter, sizeof(reporter), rfp)) reporter[0] = '\0';
    fclose(rfp);
    reporter[strcspn(reporter, "\r\n")] = '\0';
  } else {
    char *u = getenv("USER");
    strncpy(reporter, u ? u : "unknown", sizeof(reporter) - 1);
  }

  char hostname[256];
  gethostname(hostname, sizeof(hostname));
  char *dot = strchr(hostname, '.');
  if (dot) *dot = '\0';

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  struct tm *tm = gmtime(&ts.tv_sec);
  char tsbuf[32];
  strftime(tsbuf, sizeof(tsbuf), "%Y-%m-%dT%H:%M:%S", tm);
  char dttm[64];
  snprintf(dttm, sizeof(dttm), "%s.%06ld+00:00", tsbuf, ts.tv_nsec / 1000);

  char puzzle_id[128];
  snprintf(puzzle_id, sizeof(puzzle_id), "%02dx%02d_1", width, height);

  char json[1024];
  snprintf(json, sizeof(json),
    "{\"puzzle\":\"%s\",\"row_num\":%d,\"nodes\":%lld,\"best\":%d,"
    "\"solutions\":%d,\"reporter\":\"%s\",\"hostname\":\"%s\","
    "\"update_dttm\":\"%s\"}",
    puzzle_id, initial_rownum, nodes, best, solutions,
    reporter, hostname, dttm);

  /* Double-encode: wrap json string in outer quotes with inner quotes escaped,
     matching requests.post(url, json=json.dumps(payload)) behaviour. */
  char body[4096];
  char *bp = body;
  *bp++ = '"';
  for (char *s = json; *s && bp < body + sizeof(body) - 3; s++) {
    if (*s == '"') { *bp++ = '\\'; *bp++ = '"'; }
    else *bp++ = *s;
  }
  *bp++ = '"';
  *bp = '\0';

  printf("reporting result: %s\n", json);
  fflush(stdout);

  char cmd[2048];
  snprintf(cmd, sizeof(cmd),
    "wget -q -O - --http-user=reporter '--http-password=%s'"
    " --header='Content-Type: application/json'"
    " --post-data='%s'"
    " 'https://puzzlingaddiction.com/e2db/store_result'",
    password, body);

  FILE *fp = popen(cmd, "r");
  char response[1024] = {0};
  int ok = 0;
  if (fp) {
    size_t n = fread(response, 1, sizeof(response) - 1, fp);
    response[n] = '\0';
    ok = (pclose(fp) == 0);
  }
  if (ok)
    printf("result reported: %s\n", response[0] ? response : "(ok, no body)");
  else
    fprintf(stderr, "warning: result report failed: %s\n", response[0] ? response : "(no response)");
  fflush(stdout);
}

int
origmain(char *argv1, char *argv2) {
  int row, col, i, j, k, piecenum, rot, k1, k2, k3, k4, ord1, ord2,
    pos1, pos2, max_edge = 0;
  char temp_edges[9], c, tempstr[5], msg[128];
  piecelist_t *pl;
  piece_t *p;
  unsigned int rnd=0;

  core = atoi(argv1);
  snprintf(save_filename, sizeof(save_filename), "e2state-%s-%s.sav", puzzle_name, argv1);
  signal(SIGINT,  signal_handler);
  signal(SIGTERM, signal_handler);

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
  if (argv2) {
    printf("seed = %d\n", atoi(argv2));
    srand(atoi(argv2));
  } else {
    unsigned int seed = (unsigned int)(time(NULL) * 1000 + getpid() % 1000);
    printf("seed=%u\n", seed);
    srand(seed);
  }
#endif
  for(piecenum=0; piecenum<=width*height; piecenum++) {
    placed[piecenum] = (piecenum==0);
    k = 0;
    for(j=0; j<2; j++) {
      for(i=0; i<4; i++) {
	c = piece_data[piecenum*5+i];
	if (edge_c2i(c) > max_edge) {
	  max_edge = edge_c2i(c);
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
	pieces[piecenum][i].edges[j] = edge_c2i(tempstr[j]);
      }
      strcpy(pieces[piecenum][i].edgestr, tempstr);
      //printf("%s ", pieces[piecenum][i].edgestr);
    }
    //printf("\n");
  }
  //printf("piece_data = %s\n", piece_data);
  printf("max_edge = %d\n", max_edge);
  fit_size1 = max_edge+2;
  printf("fit_size1 = %d\n", fit_size1);
  fit_size2 = 1;
  for (i=0; i<4; i++) {
    fit_size2 *= fit_size1;
  }
  fit_table = calloc(fit_size2, sizeof(piecelist_t *));
  int dirs[4][2] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};
  Q = calloc(width*height, sizeof(square_t));
  restart();
  load_state(save_filename);
  initial_rownum = rownum;
  last_save_time = time(NULL);
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
  if (argc < 2) {
    fprintf(stderr, "usage: %s <core> [rownum]\n", argv[0]);
    return 1;
  }

  /* Extract puzzle name from the executable filename (first dash-delimited component). */
  char *base = strrchr(argv[0], '/');
  base = base ? base + 1 : argv[0];
  strncpy(puzzle_name, base, sizeof(puzzle_name) - 1);
  puzzle_name[sizeof(puzzle_name) - 1] = '\0';
  char *dash = strchr(puzzle_name, '-');
  if (dash) *dash = '\0';

  if (argc >= 3) {
    rownum = atoi(argv[2]);
    printf("rownum=%d (from command line)\n", rownum);
    fflush(stdout);
    origmain(argv[1], argv[2]);
  } else {
    /* Determine rownum: try save file first, then web service. */
    char savefile[256];
    snprintf(savefile, sizeof(savefile), "e2state-%s-%s.sav", puzzle_name, argv[1]);
    FILE *fp = fopen(savefile, "r");
    if (fp) {
      int ord_val, rn;
      if (fscanf(fp, "ord=%d\n", &ord_val) == 1 &&
          fscanf(fp, "rownum=%d\n", &rn) == 1) {
        rownum = rn;
        printf("rownum=%d (from save file %s)\n", rownum, savefile);
        fflush(stdout);
      }
      fclose(fp);
    }
    if (rownum < 0) {
      if (strcmp(puzzle_name, "10x10") == 0)
        rownum = fetch_rownum_from_web();

      if (rownum < 0) {
        fprintf(stderr, "error: rownum required for puzzle '%s' (no save file, no web service)\n", puzzle_name);
        return 1;
      }
    }
    origmain(argv[1], NULL);
  }

  print_status(1, 0);
  printf("nodes = %lld\n", nodes);
  printf("solutions = %d\n", solutions);
  report_result();
  if (save_filename[0])
    remove(save_filename);
  exit(0);
}
#endif
