package com.e2gen.app;

/**
 * Represents a single Eternity 2 puzzle piece.
 *
 * Each piece has four edge color values (north, east, south, west)
 * and can be rotated in 90-degree increments.
 */
public class Piece {
    public final int id;
    /** Edge colors indexed as: 0=north, 1=east, 2=south, 3=west */
    private final int[] edges;
    private int rotation; // 0, 1, 2, or 3 (multiples of 90°)

    public Piece(int id, int north, int east, int south, int west) {
        this.id = id;
        this.edges = new int[]{north, east, south, west};
        this.rotation = 0;
    }

    public int getNorth() { return edges[(0 + rotation) % 4]; }
    public int getEast()  { return edges[(1 + rotation) % 4]; }
    public int getSouth() { return edges[(2 + rotation) % 4]; }
    public int getWest()  { return edges[(3 + rotation) % 4]; }

    public int getRotation() { return rotation; }

    public void rotate() {
        rotation = (rotation + 1) % 4;
    }

    public void setRotation(int r) {
        rotation = r % 4;
    }

    /** Returns true if this piece is a border piece (has at least one 0-color edge). */
    public boolean isBorderPiece() {
        for (int e : edges) {
            if (e == 0) return true;
        }
        return false;
    }

    /** Returns true if this piece is a corner piece (has exactly two 0-color edges). */
    public boolean isCornerPiece() {
        int zeros = 0;
        for (int e : edges) {
            if (e == 0) zeros++;
        }
        return zeros == 2;
    }
}
