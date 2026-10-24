package com.pacman;

import java.awt.*;

/**
 * Labirinto clássico do Pac-Man representado como uma grade de tiles.
 *
 * Legenda:
 *   1 = parede
 *   0 = corredor com pastilha comum (dot)
 *   2 = pastilha de poder (power pellet)
 *   3 = corredor vazio (sem pastilha — área da casa dos fantasmas)
 */
public class Maze {

    public static final int COLS = GamePanel.COLS; // 19
    public static final int ROWS = GamePanel.ROWS; // 21

    // Layout 21 linhas × 19 colunas
    private static final int[][] TEMPLATE = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,2,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,2,1},
        {1,0,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,0,1},
        {1,0,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,0,1,0,1,1,1,1,1,0,1,0,1,1,0,1},
        {1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1},
        {1,1,1,1,0,1,1,1,3,1,3,1,1,1,0,1,1,1,1},
        {1,1,1,1,0,1,3,3,3,3,3,3,3,1,0,1,1,1,1},
        {1,1,1,1,0,1,3,1,1,3,1,1,3,1,0,1,1,1,1},
        {3,3,3,3,0,3,3,1,3,3,3,1,3,3,0,3,3,3,3}, // corredor central
        {1,1,1,1,0,1,3,1,1,1,1,1,3,1,0,1,1,1,1},
        {1,1,1,1,0,1,3,3,3,3,3,3,3,1,0,1,1,1,1},
        {1,1,1,1,0,1,3,1,1,1,1,1,3,1,0,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,0,1},
        {1,2,0,1,0,0,0,0,0,3,0,0,0,0,0,1,0,2,1},
        {1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1},
        {1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1},
        {1,0,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    };

    private final int[][] grid;
    private int totalDots;

    public Maze() {
        grid = new int[ROWS][COLS];
        reset();
    }

    public void reset() {
        totalDots = 0;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                grid[r][c] = TEMPLATE[r][c];
                if (grid[r][c] == 0 || grid[r][c] == 2) totalDots++;
            }
        }
    }

    public boolean isWall(int col, int row) {
        if (col < 0 || col >= COLS || row < 0 || row >= ROWS) return true;
        return grid[row][col] == 1;
    }

    /**
     * Remove a pastilha do tile (col, row) e retorna os pontos ganhos.
     * Pastilha comum = 10 pts; pastilha de poder = 50 pts.
     */
    public int collectDot(int col, int row) {
        if (col < 0 || col >= COLS || row < 0 || row >= ROWS) return 0;
        int cell = grid[row][col];
        if (cell == 0) {
            grid[row][col] = 3;
            totalDots--;
            return 10;
        }
        if (cell == 2) {
            grid[row][col] = 3;
            totalDots--;
            return 50;
        }
        return 0;
    }

    public int remainingDots() { return totalDots; }

    public void draw(Graphics2D g2) {
        int T = GamePanel.TILE_SIZE;

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                int px = c * T;
                int py = r * T;

                switch (grid[r][c]) {
                    case 1 -> drawWall(g2, px, py, T);
                    case 0 -> drawDot(g2, px, py, T);
                    case 2 -> drawPowerPellet(g2, px, py, T);
                    default -> {} // vazio / corredor sem pastilha
                }
            }
        }
    }

    private void drawWall(Graphics2D g2, int px, int py, int T) {
        // Fundo azul escuro
        g2.setColor(new Color(0, 0, 180));
        g2.fillRoundRect(px + 1, py + 1, T - 2, T - 2, 8, 8);
        // Brilho na borda
        g2.setColor(new Color(33, 33, 255));
        g2.drawRoundRect(px + 1, py + 1, T - 2, T - 2, 8, 8);
    }

    private void drawDot(Graphics2D g2, int px, int py, int T) {
        g2.setColor(new Color(255, 184, 255));
        int size = 4;
        g2.fillRect(px + T / 2 - size / 2, py + T / 2 - size / 2, size, size);
    }

    private void drawPowerPellet(Graphics2D g2, int px, int py, int T) {
        g2.setColor(new Color(255, 184, 255));
        int size = 10;
        g2.fillOval(px + T / 2 - size / 2, py + T / 2 - size / 2, size, size);
    }
}
