package com.pacman;

import java.awt.*;
import java.awt.geom.*;
import java.util.Random;

/**
 * Fantasma com IA fiel ao Pac-Man original:
 *
 *  BLINKY (vermelho) — sai imediatamente, sempre persegue o Pac-Man diretamente.
 *  PINKY  (rosa)     — sai após 30 frames, mira 4 tiles à frente do Pac-Man.
 *  INKY   (ciano)    — sai após 90 frames, usa posição de Blinky para triangular.
 *  CLYDE  (laranja)  — sai após 150 frames, persegue quando longe (>8 tiles), foge quando perto.
 *
 *  Modo SCATTER: cada fantasma vai para seu canto do labirinto.
 *  Modo FRIGHTENED: foge aleatoriamente; pisca nos últimos 2s.
 */
public class Ghost {

    public enum Type { BLINKY, PINKY, INKY, CLYDE }

    private enum Mode { HOUSE, SCATTER, CHASE, FRIGHTENED }

    private static final int SIZE              = GamePanel.TILE_SIZE - 2;
    private static final int SPEED             = 1;
    private static final int FRIGHTENED_DURATION = 60 * 7;
    private static final int FLASH_START       = 60 * 2;
    private static final int SCATTER_DURATION  = 60 * 7;
    private static final int CHASE_DURATION    = 60 * 20;

    // Tiles de saída da casa para cada tipo
    private static final int[] HOUSE_EXIT_DELAYS = { 0, 30, 90, 150 };

    // Cantos de scatter (col, row) por tipo
    private static final int[][] SCATTER_TARGETS = {
        {17, 0},  // BLINKY — canto superior direito
        {1,  0},  // PINKY  — canto superior esquerdo
        {17, 20}, // INKY   — canto inferior direito
        {1,  20}  // CLYDE  — canto inferior esquerdo
    };

    private int x, y;
    private final int startX, startY;

    private Direction direction = Direction.LEFT;
    private final Type type;
    private final Maze maze;
    private final Random rng = new Random();

    private Mode mode        = Mode.HOUSE;
    private int  modeTimer   = 0;      // tempo no modo atual
    private int  houseTimer  = 0;      // contador para sair da casa
    private boolean modePhase = false; // false=scatter, true=chase

    private int frightenedTimer = 0;
    private boolean flashing     = false;
    private boolean flashVisible = true;
    private int flashTimer       = 0;

    // Referência ao Blinky para IA do Inky
    private Ghost blinky;

    public Ghost(int tileX, int tileY, Type type, Maze maze) {
        this.type   = type;
        this.maze   = maze;
        int T = GamePanel.TILE_SIZE;
        this.startX = tileX * T + T / 2;
        this.startY = tileY * T + T / 2;
        this.x      = startX;
        this.y      = startY;
    }

    /** Fornece referência ao Blinky para que Inky possa calcular seu alvo. */
    public void setBlinky(Ghost blinky) {
        this.blinky = blinky;
    }

    // ---- Controle de estado ----

    public void frighten() {
        if (mode == Mode.HOUSE) return;
        mode            = Mode.FRIGHTENED;
        frightenedTimer = FRIGHTENED_DURATION;
        flashing        = false;
        flashVisible    = true;
        flashTimer      = 0;
        // Inverte direção ao entrar em frightened
        direction = opposite(direction);
    }

    public boolean isFrightened() { return mode == Mode.FRIGHTENED; }
    public boolean isActive()     { return mode != Mode.HOUSE; }

    public void reset() {
        x               = startX;
        y               = startY;
        mode            = Mode.HOUSE;
        modeTimer       = 0;
        houseTimer      = 0;
        modePhase       = false;
        frightenedTimer = 0;
        flashing        = false;
        flashVisible    = true;
        direction       = Direction.LEFT;
    }

    // ---- Update ----

    public void update(PacMan pacMan) {
        switch (mode) {
            case HOUSE      -> updateHouse();
            case SCATTER    -> updateMove(scatterTarget());
            case CHASE      -> updateMove(chaseTarget(pacMan));
            case FRIGHTENED -> updateFrightened();
        }
    }

    private void updateHouse() {
        houseTimer++;
        if (houseTimer >= HOUSE_EXIT_DELAYS[type.ordinal()]) {
            mode      = Mode.SCATTER;
            modeTimer = 0;
            direction = Direction.LEFT;
        }
    }

    private void updateMove(int[] target) {
        modeTimer++;

        // Alterna scatter ↔ chase
        int limit = modePhase ? CHASE_DURATION : SCATTER_DURATION;
        if (modeTimer >= limit) {
            modeTimer  = 0;
            modePhase  = !modePhase;
            mode       = modePhase ? Mode.CHASE : Mode.SCATTER;
            direction  = opposite(direction);
        }

        if (isOnTileCenter()) {
            direction = bestDirection(getTileX(), getTileY(), target[0], target[1]);
        }
        moveForward();
    }

    private void updateFrightened() {
        frightenedTimer--;

        if (frightenedTimer <= FLASH_START) {
            flashing = true;
            if (++flashTimer >= 10) {
                flashVisible = !flashVisible;
                flashTimer   = 0;
            }
        }

        if (frightenedTimer <= 0) {
            mode         = modePhase ? Mode.CHASE : Mode.SCATTER;
            modeTimer    = 0;
            flashing     = false;
            flashVisible = true;
        }

        if (isOnTileCenter()) {
            direction = randomDirection();
        }
        moveForward();
    }

    private void moveForward() {
        x += direction.dx() * SPEED;
        y += direction.dy() * SPEED;
        if (x < 0)                x = GamePanel.WIDTH;
        if (x > GamePanel.WIDTH)  x = 0;
    }

    // ---- Alvos de IA ----

    /** Tile alvo no modo scatter: canto designado. */
    private int[] scatterTarget() {
        return SCATTER_TARGETS[type.ordinal()];
    }

    /** Tile alvo no modo chase: personalidade de cada fantasma. */
    private int[] chaseTarget(PacMan pac) {
        int px = pac.getTileX();
        int py = pac.getTileY();

        return switch (type) {
            // Blinky: persegue diretamente
            case BLINKY -> new int[]{ px, py };

            // Pinky: mira 4 tiles à frente do Pac-Man
            case PINKY -> new int[]{
                px + pac.getDirection().dx() * 4,
                py + pac.getDirection().dy() * 4
            };

            // Inky: triangula entre Blinky e 2 tiles à frente do Pac-Man
            case INKY -> {
                int pivotX = px + pac.getDirection().dx() * 2;
                int pivotY = py + pac.getDirection().dy() * 2;
                if (blinky != null) {
                    int bx = blinky.getTileX();
                    int by = blinky.getTileY();
                    yield new int[]{ pivotX + (pivotX - bx), pivotY + (pivotY - by) };
                }
                yield new int[]{ pivotX, pivotY };
            }

            // Clyde: persegue quando > 8 tiles, vai para canto quando perto
            case CLYDE -> {
                double dist = Math.hypot(getTileX() - px, getTileY() - py);
                yield dist > 8
                    ? new int[]{ px, py }
                    : SCATTER_TARGETS[Type.CLYDE.ordinal()];
            }
        };
    }

    // ---- Navegação ----

    private Direction bestDirection(int fromX, int fromY, int targetX, int targetY) {
        Direction[] dirs = { Direction.UP, Direction.DOWN, Direction.LEFT, Direction.RIGHT };
        Direction opp    = opposite(direction);
        Direction best   = null;
        double bestDist  = Double.MAX_VALUE;

        for (Direction d : dirs) {
            if (d == opp) continue;
            int nx = fromX + d.dx();
            int ny = fromY + d.dy();
            if (!maze.isWall(nx, ny)) {
                double dist = Math.hypot(nx - targetX, ny - targetY);
                if (dist < bestDist) {
                    bestDist = dist;
                    best     = d;
                }
            }
        }
        return best != null ? best : direction;
    }

    private Direction randomDirection() {
        Direction[] dirs = { Direction.UP, Direction.DOWN, Direction.LEFT, Direction.RIGHT };
        Direction opp    = opposite(direction);
        java.util.List<Direction> valid = new java.util.ArrayList<>();
        for (Direction d : dirs) {
            if (d != opp && !maze.isWall(getTileX() + d.dx(), getTileY() + d.dy()))
                valid.add(d);
        }
        return valid.isEmpty() ? direction : valid.get(rng.nextInt(valid.size()));
    }

    private static Direction opposite(Direction d) {
        return switch (d) {
            case LEFT  -> Direction.RIGHT;
            case RIGHT -> Direction.LEFT;
            case UP    -> Direction.DOWN;
            case DOWN  -> Direction.UP;
            default    -> Direction.NONE;
        };
    }

    private boolean isOnTileCenter() {
        int T = GamePanel.TILE_SIZE;
        return (x % T == T / 2) && (y % T == T / 2);
    }

    // ---- Desenho ----

    public void draw(Graphics2D g2) {
        if (mode == Mode.HOUSE) return;
        if (flashing && !flashVisible) return;

        int drawX = x - SIZE / 2;
        int drawY = y - SIZE / 2;
        int w = SIZE, h = SIZE;

        Color bodyColor = isFrightened()
                ? (flashing ? Color.WHITE : new Color(0, 0, 200))
                : ghostColor();

        // Corpo
        GeneralPath body = new GeneralPath();
        int halfW = w / 2;
        body.moveTo(drawX, drawY + halfW);
        body.append(new Arc2D.Double(drawX, drawY, w, w, 0, 180, Arc2D.OPEN), true);
        body.lineTo(drawX + w, drawY + h);
        int seg = w / 3;
        body.lineTo(drawX + w - seg / 2,           drawY + h - seg / 2);
        body.lineTo(drawX + w - seg,               drawY + h);
        body.lineTo(drawX + w - seg - seg / 2,     drawY + h - seg / 2);
        body.lineTo(drawX + halfW,                 drawY + h);
        body.lineTo(drawX + seg / 2 + 2,           drawY + h - seg / 2);
        body.lineTo(drawX + seg / 2,               drawY + h - seg / 2);
        body.lineTo(drawX,                         drawY + h);
        body.lineTo(drawX, drawY + halfW);
        body.closePath();

        g2.setColor(bodyColor);
        g2.fill(body);

        drawEyes(g2, drawX, drawY, w, h);
    }

    private void drawEyes(Graphics2D g2, int drawX, int drawY, int w, int h) {
        if (isFrightened()) {
            g2.setColor(Color.WHITE);
            g2.fillRect(drawX + w / 5,          drawY + h / 3, w / 6, h / 6);
            g2.fillRect(drawX + w / 2 + w / 10, drawY + h / 3, w / 6, h / 6);

            Color mouthColor = flashing ? Color.RED : Color.WHITE;
            g2.setColor(mouthColor);
            int mx = drawX + w / 6, my = drawY + h * 2 / 3, ms = w / 5;
            g2.drawLine(mx,            my,          mx + ms / 2,           my - ms / 3);
            g2.drawLine(mx + ms / 2,   my - ms / 3, mx + ms,               my);
            g2.drawLine(mx + ms,       my,          mx + ms + ms / 2,      my - ms / 3);
            g2.drawLine(mx + ms + ms / 2, my - ms / 3, mx + ms * 2,        my);
            return;
        }

        int eyeR = w / 6;
        int lx = drawX + w / 4;
        int rx = drawX + w * 3 / 4 - eyeR;
        int ey = drawY + h / 3;

        g2.setColor(Color.WHITE);
        g2.fillOval(lx - eyeR, ey - eyeR, eyeR * 2, eyeR * 2);
        g2.fillOval(rx - eyeR, ey - eyeR, eyeR * 2, eyeR * 2);

        g2.setColor(new Color(0, 0, 180));
        g2.fillOval(lx - eyeR / 2, ey - eyeR / 2, eyeR, eyeR);
        g2.fillOval(rx - eyeR / 2, ey - eyeR / 2, eyeR, eyeR);
    }

    private Color ghostColor() {
        return switch (type) {
            case BLINKY -> Color.RED;
            case PINKY  -> new Color(255, 184, 255);
            case INKY   -> Color.CYAN;
            case CLYDE  -> new Color(255, 184, 71);
        };
    }

    public int getTileX() { return x / GamePanel.TILE_SIZE; }
    public int getTileY() { return y / GamePanel.TILE_SIZE; }
    public int getX()     { return x; }
    public int getY()     { return y; }
}
