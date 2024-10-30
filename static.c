  for(ord1=0; ord1<width*height; ord1++) {
    int row1, col1, row2, col2, dir;
    if (ord1 > best) {
      print_puzz2(ord1);
      best = ord1;
    }
    if (!Q[ord1].active) {
      pos1 = ord2pos[ord1];
      row1 = pos1/width;
      col1 = pos1%width;
      k = 0;
      for(dir=0; dir<4; dir++) {
	k *= fit_size1;
	row2 = row1 + dirs[dir][0];
	col2 = col1 + dirs[dir][1];
	if (row2 < 0 || row2 >= width || col2 < 0 || col2 >= height) {
	  // off puzzle
	  continue;
	}
	pos2 = row2*width+col2;
	ord2 = pos2ord[pos2];
	if (ord2 >= ord1) {
	  // unplaced piece
	  k += fit_size1-1;
	  continue;
	}
	p = Q[ord2].pieces->piece;
	// match piece
	k += p->edges[(dir+2)%4];
      }
      Q[ord1].pieces = fit_table[k];
      Q[ord1].active = TRUE;
    }
    if (Q[ord1].pieces->piece) {
      placed[Q[ord1].pieces->piece->piecenum] -= 1;
    }
    Q[ord1].pieces = Q[ord1].pieces->next;
    if (!Q[ord1].pieces) {
      Q[ord1].active = FALSE;
      ord1 -= 2;
      continue;
    }
    p = Q[ord1].pieces->piece;
    if (placed[p->piecenum] || (ord1 == 0 && (p->piecenum != 139 || p->rot != 2))) {
      placed[Q[ord1].pieces->piece->piecenum]++;
      ord1 -= 1;
      continue;
    }
    nodes += 1;
    if (nodes % 100000000 == 0) {
      print_status();
    }
    placed[Q[ord1].pieces->piece->piecenum] = 1;
  }
