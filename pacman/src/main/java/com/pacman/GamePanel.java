package com.pacman;

import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import java.awt.*;
import java.awt.event.*;

/**
 * Painel principal: game loop (60fps), renderização, input, colisões.
 * Integra: 4 fantasmas com IA, frutas bônus, som, menu de pausa.
 */
public class GamePanel extends JPanel implements KeyListener {

    public static final int TILE_SIZE = 24;
    public static final int COLS      = 19;
    public static final int ROWS      = 21;
    public static final int WIDTH     = COLS * TILE_SIZE;
    public static final int HEIGHT    = ROWS * TILE_SIZE + 40;

    private static final int TARGET_FPS = 60;
    private static final long FRAME_NS  = 1_000_000_000L / TARGET_FPS;

    // Pastilhas coletadas que disparam a fruta
    private static final int FRUIT_TRIGGER_1 = 70;
    private static final int FRUIT_TRIGGER_2 = 170;

    private Thread  gameThread;
    private boolean running;

    private Maze         maze;
    private PacMan       pacMan;
    private Ghost[]      ghosts;
    private BonusFruit   bonusFruit;
    private SoundManager sound;
    private PauseMenu    pauseMenu;

    private int  score;
    private int  lives;
    private int  dotsCollected;
    private int  ghostEatMulti;   // 200 → 400 → 800 → 1600
    private boolean gameOver;
    private boolean paused;
    private boolean levelComplete;
    private int     levelCompleteTimer;

    // Pontuação flutuante ao comer fantasma/fruta
    private int floatScore;
    private int floatX, floatY, floatTimer;

    public GamePanel() {
        setPreferredSize(new Dimension(WIDTH, HEIGHT));
        setBackground(Color.BLACK);
        setFocusable(true);
        addKeyListener(this);
        sound     = new SoundManager();
        pauseMenu = new PauseMenu(sound);
        init();
    }

    private void init() {
        score              = 0;
        lives              = 3;
        dotsCollected      = 0;
        ghostEatMulti      = 200;
        gameOver           = false;
        paused             = false;
        levelComplete      = false;
        levelCompleteTimer = 0;
        bonusFruit         = null;
        floatTimer         = 0;

        maze   = new Maze();
        pacMan = new PacMan(9, 15, maze);

        Ghost blinky = new Ghost(9, 9, Ghost.Type.BLINKY, maze);
        Ghost pinky  = new Ghost(9, 9, Ghost.Type.PINKY,  maze);
        Ghost inky   = new Ghost(9, 9, Ghost.Type.INKY,   maze);
        Ghost clyde  = new Ghost(9, 9, Ghost.Type.CLYDE,  maze);
        inky.setBlinky(blinky);
        ghosts = new Ghost[]{ blinky, pinky, inky, clyde };

        sound.playLevelStart();
        sound.startMusic();
    }

    public void start() {
        running    = true;
        gameThread = new Thread(this::gameLoop);
        gameThread.setDaemon(true);
        gameThread.start();
    }

    // ---- Game loop ----

    private void gameLoop() {
        long lastTime = System.nanoTime();
        while (running) {
            long now   = System.nanoTime();
            long delta = now - lastTime;
            if (delta >= FRAME_NS) {
                lastTime = now;
                update();
                repaint();
            }
            try { Thread.sleep(1); } catch (InterruptedException ignored) {}
        }
    }

    private void update() {
        if (gameOver || paused) return;

        if (levelComplete) {
            if (++levelCompleteTimer > 90) {
                maze.reset();
                dotsCollected      = 0;
                bonusFruit         = null;
                ghostEatMulti      = 200;
                levelComplete      = false;
                levelCompleteTimer = 0;
                for (Ghost g : ghosts) g.reset();
                pacMan.reset();
                sound.playLevelStart();
                sound.startMusic();
            }
            return;
        }

        pacMan.update();
        for (Ghost g : ghosts) g.update(pacMan);
        if (bonusFruit != null) bonusFruit.update();
        checkCollisions();

        if (maze.remainingDots() == 0) {
            levelComplete = true;
            sound.stopMusic();
        }
    }

    private void checkCollisions() {
        int px = pacMan.getTileX();
        int py = pacMan.getTileY();

        // Coleta pastilha
        int gained = maze.collectDot(px, py);
        if (gained > 0) {
            score += gained;
            dotsCollected++;
            sound.playWaka(pacMan.isMouthOpen());

            if (gained == 50) {
                // Power pellet: ativa modo frightened
                sound.stopMusic();
                sound.playPowerPellet();
                ghostEatMulti = 200;
                for (Ghost g : ghosts) g.frighten();
                new Thread(() -> {
                    try { Thread.sleep(7200); } catch (InterruptedException ignored) {}
                    if (!gameOver && !paused) sound.startMusic();
                }).start();
            }

            // Dispara fruta bônus
            if (dotsCollected == FRUIT_TRIGGER_1) {
                bonusFruit = new BonusFruit(BonusFruit.Type.CHERRY);
            } else if (dotsCollected == FRUIT_TRIGGER_2) {
                bonusFruit = new BonusFruit(BonusFruit.Type.STRAWBERRY);
            }
        }

        // Coleta fruta
        if (bonusFruit != null && bonusFruit.isActive()
                && bonusFruit.getTileX() == px && bonusFruit.getTileY() == py) {
            int pts = bonusFruit.getPoints();
            score += pts;
            bonusFruit.collect();
            sound.playFruit();
            showFloatScore(pts, px * TILE_SIZE, py * TILE_SIZE);
        }

        // Colisão com fantasmas
        for (Ghost g : ghosts) {
            if (!g.isActive()) continue;
            if (g.getTileX() == px && g.getTileY() == py) {
                if (g.isFrightened()) {
                    score += ghostEatMulti;
                    showFloatScore(ghostEatMulti, g.getX(), g.getY());
                    ghostEatMulti = Math.min(ghostEatMulti * 2, 1600);
                    g.reset();
                    sound.playEatGhost();
                } else if (!gameOver) {
                    lives--;
                    sound.stopMusic();
                    sound.playDeath();
                    if (lives <= 0) {
                        gameOver = true;
                    } else {
                        new Thread(() -> {
                            try { Thread.sleep(1000); } catch (InterruptedException ignored) {}
                            pacMan.reset();
                            for (Ghost gh : ghosts) gh.reset();
                            sound.startMusic();
                        }).start();
                    }
                }
            }
        }
    }

    private void showFloatScore(int pts, int x, int y) {
        floatScore = pts;
        floatX     = x;
        floatY     = y;
        floatTimer = 80;
    }

    // ---- Renderização ----

    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        Graphics2D g2 = (Graphics2D) g;
        g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        maze.draw(g2);
        if (bonusFruit != null) bonusFruit.draw(g2);
        pacMan.draw(g2);
        for (Ghost ghost : ghosts) ghost.draw(g2);
        drawFloatScore(g2);
        drawHUD(g2);

        if (levelComplete) drawLevelComplete(g2);
        if (gameOver)      drawGameOver(g2);
        if (paused)        pauseMenu.draw(g2, WIDTH, HEIGHT);
    }

    private void drawHUD(Graphics2D g2) {
        int y = ROWS * TILE_SIZE;
        g2.setColor(Color.BLACK);
        g2.fillRect(0, y, WIDTH, 40);

        g2.setColor(Color.WHITE);
        g2.setFont(new Font("Arial", Font.BOLD, 14));
        g2.drawString("SCORE: " + score, 10, y + 24);

        for (int i = 0; i < lives; i++) {
            int lx = WIDTH - 28 - i * 22;
            int ly = y + 6;
            g2.setColor(Color.YELLOW);
            g2.fillArc(lx, ly, 14, 14, 30, 300);
        }

        g2.setColor(new Color(120, 120, 120));
        g2.setFont(new Font("Arial", Font.PLAIN, 11));
        g2.drawString("[P] Pausar", WIDTH / 2 - 28, y + 24);
    }

    private void drawFloatScore(Graphics2D g2) {
        if (floatTimer <= 0) return;
        float alpha = Math.min(1f, floatTimer / 40f);
        g2.setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, alpha));
        g2.setColor(Color.CYAN);
        g2.setFont(new Font("Arial", Font.BOLD, 13));
        g2.drawString("+" + floatScore, floatX, floatY - (80 - floatTimer) / 4);
        g2.setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 1f));
        floatTimer--;
    }

    private void drawGameOver(Graphics2D g2) {
        g2.setColor(new Color(0, 0, 0, 160));
        g2.fillRect(0, 0, WIDTH, HEIGHT);
        g2.setColor(Color.RED);
        g2.setFont(new Font("Arial", Font.BOLD, 32));
        FontMetrics fm = g2.getFontMetrics();
        String msg = "GAME OVER";
        g2.drawString(msg, (WIDTH - fm.stringWidth(msg)) / 2, HEIGHT / 2);
        g2.setColor(Color.WHITE);
        g2.setFont(new Font("Arial", Font.PLAIN, 16));
        fm = g2.getFontMetrics();
        String sub = "Pressione R para reiniciar";
        g2.drawString(sub, (WIDTH - fm.stringWidth(sub)) / 2, HEIGHT / 2 + 36);
    }

    private void drawLevelComplete(Graphics2D g2) {
        g2.setColor(new Color(0, 0, 0, 120));
        g2.fillRect(0, 0, WIDTH, HEIGHT);
        g2.setColor(Color.YELLOW);
        g2.setFont(new Font("Arial", Font.BOLD, 26));
        FontMetrics fm = g2.getFontMetrics();
        String msg = "FASE COMPLETA!";
        g2.drawString(msg, (WIDTH - fm.stringWidth(msg)) / 2, HEIGHT / 2);
    }

    // ---- Input ----

    @Override
    public void keyPressed(KeyEvent e) {
        int key = e.getKeyCode();

        if (key == KeyEvent.VK_R && gameOver) {
            sound.stopMusic();
            init();
            return;
        }

        if ((key == KeyEvent.VK_P || key == KeyEvent.VK_ESCAPE) && !gameOver) {
            paused = !paused;
            if (paused) { sound.pauseMusic(); pauseMenu.resetSelection(); }
            else          sound.resumeMusic();
            return;
        }

        if (paused) {
            PauseMenu.Action action = pauseMenu.handleKey(key);
            switch (action) {
                case RESUME  -> { paused = false; sound.resumeMusic(); }
                case RESTART -> { sound.stopMusic(); paused = false; init(); }
                case QUIT    -> {
                    sound.shutdown();
                    SwingUtilities.getWindowAncestor(this).dispose();
                    System.exit(0);
                }
                default -> {}
            }
            return;
        }

        switch (key) {
            case KeyEvent.VK_LEFT,  KeyEvent.VK_A -> pacMan.setNextDirection(Direction.LEFT);
            case KeyEvent.VK_RIGHT, KeyEvent.VK_D -> pacMan.setNextDirection(Direction.RIGHT);
            case KeyEvent.VK_UP,    KeyEvent.VK_W -> pacMan.setNextDirection(Direction.UP);
            case KeyEvent.VK_DOWN,  KeyEvent.VK_S -> pacMan.setNextDirection(Direction.DOWN);
        }
    }

    @Override public void keyReleased(KeyEvent e) {}
    @Override public void keyTyped(KeyEvent e) {}
}
