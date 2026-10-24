package com.pacman;

public enum Direction {
    LEFT, RIGHT, UP, DOWN, NONE;

    public int dx() {
        return switch (this) { case LEFT -> -1; case RIGHT -> 1; default -> 0; };
    }

    public int dy() {
        return switch (this) { case UP -> -1; case DOWN -> 1; default -> 0; };
    }

    /** Ângulo de rotação em graus para orientar o Pac-Man desenhado olhando para a DIREITA. */
    public double angle() {
        return switch (this) {
            case RIGHT -> 0;
            case DOWN  -> 90;
            case LEFT  -> 180;
            case UP    -> 270;
            default    -> 0;
        };
    }
}
