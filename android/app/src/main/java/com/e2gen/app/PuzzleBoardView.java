package com.e2gen.app;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.util.AttributeSet;
import android.view.View;

import androidx.core.content.ContextCompat;

/**
 * Custom view that renders the Eternity 2 puzzle board.
 *
 * Each cell is drawn as a square with four triangular edge regions,
 * each coloured according to the corresponding piece edge value.
 */
public class PuzzleBoardView extends View {

    private static final int CELL_PX = 48; // dp-equivalent size per cell in pixels

    // Palette of 23 colours (index 0 = border grey, 1-22 = puzzle colours)
    private static final int[] PALETTE = {
        0xFF9E9E9E, // 0  grey  (border)
        0xFFE53935, // 1  red
        0xFF8E24AA, // 2  purple
        0xFF1E88E5, // 3  blue
        0xFF00ACC1, // 4  cyan
        0xFF43A047, // 5  green
        0xFFE2B400, // 6  yellow
        0xFFFF7043, // 7  deep orange
        0xFF6D4C41, // 8  brown
        0xFF546E7A, // 9  blue grey
        0xFFEC407A, // 10 pink
        0xFF26C6DA, // 11 light cyan
        0xFF66BB6A, // 12 light green
        0xFFFFCA28, // 13 amber
        0xFF29B6F6, // 14 light blue
        0xFFAB47BC, // 15 medium purple
        0xFF26A69A, // 16 teal
        0xFFEF5350, // 17 light red
        0xFF7E57C2, // 18 deep purple
        0xFF78909C, // 19 steel blue
        0xFFD4E157, // 20 lime
        0xFFFF8A65, // 21 light orange
        0xFFA5D6A7, // 22 light green 2
    };

    private PuzzleBoard board;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint borderPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

    public PuzzleBoardView(Context context) {
        super(context);
        init();
    }

    public PuzzleBoardView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        borderPaint.setColor(0xFF000000);
        borderPaint.setStyle(Paint.Style.STROKE);
        borderPaint.setStrokeWidth(2f);
    }

    public void setBoard(PuzzleBoard board) {
        this.board = board;
        requestLayout();
        invalidate();
    }

    @Override
    protected void onMeasure(int widthSpec, int heightSpec) {
        if (board == null) {
            super.onMeasure(widthSpec, heightSpec);
            return;
        }
        int size = board.getSize() * CELL_PX;
        setMeasuredDimension(size, size);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (board == null) return;

        int n = board.getSize();
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                drawCell(canvas, row, col);
            }
        }
    }

    private void drawCell(Canvas canvas, int row, int col) {
        float left   = col * CELL_PX;
        float top    = row * CELL_PX;
        float right  = left + CELL_PX;
        float bottom = top  + CELL_PX;
        float cx = (left + right) / 2f;
        float cy = (top + bottom) / 2f;

        Piece piece = board.getPieceAt(row, col);

        if (piece == null) {
            // Empty cell
            paint.setColor(0xFFE0E0E0);
            paint.setStyle(Paint.Style.FILL);
            canvas.drawRect(left, top, right, bottom, paint);
        } else {
            drawTriangle(canvas, cx, cy, left, top, right, top,    piece.getNorth()); // north
            drawTriangle(canvas, cx, cy, right, top, right, bottom, piece.getEast());  // east
            drawTriangle(canvas, cx, cy, right, bottom, left, bottom, piece.getSouth()); // south
            drawTriangle(canvas, cx, cy, left, bottom, left, top,   piece.getWest());  // west
        }

        // Cell border
        canvas.drawRect(left, top, right, bottom, borderPaint);
    }

    private void drawTriangle(Canvas canvas,
                               float cx, float cy,
                               float x1, float y1,
                               float x2, float y2,
                               int colorIndex) {
        int color = colorIndex >= 0 && colorIndex < PALETTE.length
                ? PALETTE[colorIndex] : PALETTE[0];
        paint.setColor(color);
        paint.setStyle(Paint.Style.FILL);

        Path path = new Path();
        path.moveTo(cx, cy);
        path.lineTo(x1, y1);
        path.lineTo(x2, y2);
        path.close();
        canvas.drawPath(path, paint);
    }
}
