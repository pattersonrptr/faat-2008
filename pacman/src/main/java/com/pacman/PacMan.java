package com.pacman;

import java.awt.*;
import java.awt.geom.*;

/**
 * Pac-Man desenhado com Java2D:
 *  - Apenas 1 sprite base (olhando para a direita)
 *  - Rotação via Graphics2D.rotate() para as 4 direções
 *  - Animação de boca alterna entre aberta (30°) e fechada (0°) a cada ANIM_FRAMES frames
 */
public class PacMan {

    private static final int SIZE           = GamePanel.TILE_SIZE - 2;
    private static final int SPEED          = 2; // pixels por frame
    private static final int ANIM_FRAMES    = 8; // frames entre cada troca de boca
    private static final int MOUTH_ANGLE    = 40; // graus da abertura da boca

    // Posição em pixels (centro do tile inicial)
    private int x, y;
    private int startX, startY;

    private Direction direction     = Direction.RIGHT;
    private Direction nextDirection = Direction.RIGHT;

    private boolean mouthOpen  = true;
    private int     animTimer  = 0;

    private final Maze maze;

    public PacMan(int tileX, int tileY, Maze maze) {
        this.maze   = maze;
        this.startX = tileX * GamePanel.TILE_SIZE + GamePanel.TILE_SIZE / 2;
        this.startY = tileY * GamePanel.TILE_SIZE + GamePanel.TILE_SIZE / 2;
        this.x      = startX;
        this.y      = startY;
    }

    public void reset() {
        x         = startX;
        y         = startY;
        direction = Direction.RIGHT;
        nextDirection = Direction.RIGHT;
        mouthOpen = true;
        animTimer = 0;
    }

    public void setNextDirection(Direction d) {
        nextDirection = d;
    }

    public void update() {
        // Tenta trocar de direção se o próximo tile nessa direção não é parede
        if (nextDirection != direction && canMoveIn(nextDirection)) {
            direction = nextDirection;
        }

        // Move se não há parede
        if (canMoveIn(direction)) {
            x += direction.dx() * SPEED;
            y += direction.dy() * SPEED;
            // Túnel lateral
            if (x < 0)              x = GamePanel.WIDTH;
            if (x > GamePanel.WIDTH) x = 0;
        }

        // Animação de boca
        animTimer++;
        if (animTimer >= ANIM_FRAMES) {
            mouthOpen = !mouthOpen;
            animTimer = 0;
        }
    }

    /** Verifica se o Pac-Man pode se mover na direção informada (tile a frente não é parede). */
    private boolean canMoveIn(Direction d) {
        int T = GamePanel.TILE_SIZE;
        int half = SIZE / 2;

        // Ponto de verificação: borda frontal do pac-man + deslocamento futuro
        int checkX = x + d.dx() * (half + SPEED);
        int checkY = y + d.dy() * (half + SPEED);

        int tileX = checkX / T;
        int tileY = checkY / T;
        return !maze.isWall(tileX, tileY);
    }

    public void draw(Graphics2D g2) {
        int T       = GamePanel.TILE_SIZE;
        int drawX   = x - SIZE / 2;
        int drawY   = y - SIZE / 2;
        double angle = Math.toRadians(direction.angle());

        // Salva transformação atual e aplica rotação em torno do centro
        AffineTransform old = g2.getTransform();
        g2.rotate(angle, x, y);

        if (mouthOpen) {
            g2.setColor(Color.YELLOW);
            g2.fillArc(drawX, drawY, SIZE, SIZE,
                       MOUTH_ANGLE / 2,
                       360 - MOUTH_ANGLE);
        } else {
            g2.setColor(Color.YELLOW);
            g2.fillOval(drawX, drawY, SIZE, SIZE);
        }

        // Olho
        int eyeX = x + SIZE / 5;
        int eyeY = y - SIZE / 4;
        g2.setColor(Color.BLACK);
        g2.fillOval(eyeX - 2, eyeY - 2, 4, 4);

        g2.setTransform(old);
    }

    // ---- Acessores ----

    public int       getTileX()     { return x / GamePanel.TILE_SIZE; }
    public int       getTileY()     { return y / GamePanel.TILE_SIZE; }
    public int       getX()         { return x; }
    public int       getY()         { return y; }
    public Direction getDirection() { return direction; }
    public boolean   isMouthOpen()  { return mouthOpen; }
}
