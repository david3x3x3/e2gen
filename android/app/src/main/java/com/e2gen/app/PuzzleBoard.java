package com.e2gen.app;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;

/**
 * Holds the state of an Eternity 2 puzzle board.
 *
 * The board is a square grid of {@link Piece} objects. A null cell means
 * no piece has been placed there yet.
 */
public class PuzzleBoard {

    public static final int NUM_COLORS = 22; // colours 1-22; 0 = border/grey

    private final int size;
    private final Piece[] pieces;
    private final Piece[][] grid;
    private final boolean[] placed;

    public PuzzleBoard(int size) {
        this.size = size;
        int total = size * size;
        this.pieces = new Piece[total];
        this.grid = new Piece[size][size];
        this.placed = new boolean[total];
        generatePieces();
    }

    // -------------------------------------------------------------------------
    // Generation
    // -------------------------------------------------------------------------

    private void generatePieces() {
        Random rng = new Random(42);
        int total = size * size;

        for (int i = 0; i < total; i++) {
            boolean top    = (i / size == 0);
            boolean bottom = (i / size == size - 1);
            boolean left   = (i % size == 0);
            boolean right  = (i % size == size - 1);

            int north = (top)    ? 0 : randomColor(rng);
            int east  = (right)  ? 0 : randomColor(rng);
            int south = (bottom) ? 0 : randomColor(rng);
            int west  = (left)   ? 0 : randomColor(rng);

            pieces[i] = new Piece(i, north, east, south, west);
        }

        // Shuffle so the pieces arrive in random order
        List<Piece> list = new ArrayList<>();
        Collections.addAll(list, pieces);
        Collections.shuffle(list, rng);
        for (int i = 0; i < total; i++) {
            pieces[i] = list.get(i);
        }
    }

    private int randomColor(Random rng) {
        return 1 + rng.nextInt(NUM_COLORS);
    }

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    public int getSize() { return size; }

    public Piece getPieceAt(int row, int col) {
        return grid[row][col];
    }

    /** Returns the unplaced piece pool. */
    public List<Piece> getUnplacedPieces() {
        List<Piece> result = new ArrayList<>();
        for (int i = 0; i < pieces.length; i++) {
            if (!placed[i]) result.add(pieces[i]);
        }
        return result;
    }

    public boolean placePiece(Piece piece, int row, int col) {
        if (grid[row][col] != null) return false;
        grid[row][col] = piece;
        placed[piece.id] = true;
        return true;
    }

    public void clearBoard() {
        for (int r = 0; r < size; r++) {
            for (int c = 0; c < size; c++) {
                if (grid[r][c] != null) {
                    placed[grid[r][c].id] = false;
                    grid[r][c] = null;
                }
            }
        }
    }

    /** Checks whether placing {@code piece} at (row, col) is locally consistent. */
    public boolean isValidPlacement(Piece piece, int row, int col) {
        // Check north neighbour
        if (row > 0) {
            Piece north = grid[row - 1][col];
            if (north != null && north.getSouth() != piece.getNorth()) return false;
        } else {
            if (piece.getNorth() != 0) return false;
        }
        // Check west neighbour
        if (col > 0) {
            Piece west = grid[row][col - 1];
            if (west != null && west.getEast() != piece.getWest()) return false;
        } else {
            if (piece.getWest() != 0) return false;
        }
        // Check south border
        if (row == size - 1 && piece.getSouth() != 0) return false;
        // Check east border
        if (col == size - 1 && piece.getEast() != 0) return false;

        return true;
    }

    /** Returns the number of pieces currently on the board. */
    public int placedCount() {
        int count = 0;
        for (int r = 0; r < size; r++)
            for (int c = 0; c < size; c++)
                if (grid[r][c] != null) count++;
        return count;
    }
}
