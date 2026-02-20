package com.e2gen.app;

import android.os.Bundle;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.e2gen.app.databinding.ActivityPuzzleBinding;

/**
 * Displays the puzzle board and provides controls to solve it.
 */
public class PuzzleActivity extends AppCompatActivity {

    public static final String EXTRA_BOARD_SIZE = "board_size";

    private ActivityPuzzleBinding binding;
    private PuzzleBoard board;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityPuzzleBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        int size = getIntent().getIntExtra(EXTRA_BOARD_SIZE, 16);
        board = new PuzzleBoard(size);
        binding.puzzleBoardView.setBoard(board);

        binding.btnSolve.setOnClickListener(v -> startSolving());
        binding.btnNewPuzzle.setOnClickListener(v -> newPuzzle(size));

        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        }
    }

    private void startSolving() {
        // Simple greedy placement for demonstration
        board.clearBoard();
        int size = board.getSize();
        boolean placed = false;

        for (int row = 0; row < size; row++) {
            for (int col = 0; col < size; col++) {
                for (Piece piece : board.getUnplacedPieces()) {
                    for (int r = 0; r < 4; r++) {
                        piece.setRotation(r);
                        if (board.isValidPlacement(piece, row, col)) {
                            board.placePiece(piece, row, col);
                            placed = true;
                            break;
                        }
                    }
                    if (placed) { placed = false; break; }
                }
            }
        }

        binding.puzzleBoardView.invalidate();
        int count = board.placedCount();
        int total = size * size;
        Toast.makeText(this, "Placed " + count + " / " + total + " pieces", Toast.LENGTH_SHORT).show();
    }

    private void newPuzzle(int size) {
        board = new PuzzleBoard(size);
        binding.puzzleBoardView.setBoard(board);
    }

    @Override
    public boolean onSupportNavigateUp() {
        finish();
        return true;
    }
}
