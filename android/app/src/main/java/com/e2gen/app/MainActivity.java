package com.e2gen.app;

import android.content.Intent;
import android.os.Bundle;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import com.e2gen.app.databinding.ActivityMainBinding;

/**
 * Entry point of the E2Gen app.
 *
 * Lets the user generate a new puzzle and navigate to the board view.
 */
public class MainActivity extends AppCompatActivity {

    private static final int DEFAULT_SIZE = 16;

    private ActivityMainBinding binding;
    private PuzzleBoard currentBoard;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        binding.btnGenerate.setOnClickListener(v -> generatePuzzle());
        binding.btnViewBoard.setOnClickListener(v -> openBoard());

        // Generate an initial puzzle on startup
        generatePuzzle();
    }

    private void generatePuzzle() {
        currentBoard = new PuzzleBoard(DEFAULT_SIZE);
        updateInfoCard();
        binding.tvStatus.setText(getString(R.string.label_status_idle));
    }

    private void updateInfoCard() {
        if (currentBoard == null) return;
        int size = currentBoard.getSize();
        binding.tvBoardSize.setText("Board Size: " + size + "×" + size);
        binding.tvPieces.setText("Pieces: " + (size * size));
    }

    private void openBoard() {
        if (currentBoard == null) return;
        // Pass board size; PuzzleActivity will re-create the board for now
        Intent intent = new Intent(this, PuzzleActivity.class);
        intent.putExtra(PuzzleActivity.EXTRA_BOARD_SIZE, currentBoard.getSize());
        startActivity(intent);
    }
}
