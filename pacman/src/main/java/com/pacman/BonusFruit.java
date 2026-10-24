package com.pacman;

import java.awt.*;
import java.awt.geom.*;

/**
 * Fruta bônus que aparece no centro do labirinto após coletar
 * 70 e 170 pastilhas. Desaparece após 10 segundos ou ao ser coletada.
 *
 * Sprites desenhadas com Java2D, baseadas nos SVGs originais.
 */
public class BonusFruit {

    public enum Type {
        CHERRY(100), STRAWBERRY(300), ORANGE(500), APPLE(700),
        MELON(1000), GALAXIAN(2000), BELL(3000), KEY(5000);

        public final int points;
        Type(int points) { this.points = points; }
    }

    private static final int TILE_X    = 9;   // centro do labirinto
    private static final int TILE_Y    = 17;
    private static final int LIFETIME  = 60 * 10; // 10 segundos

    private final Type type;
    private int  timer;
    private boolean active;

    public BonusFruit(Type type) {
        this.type   = type;
        this.timer  = LIFETIME;
        this.active = true;
    }

    public void update() {
        if (!active) return;
        if (--timer <= 0) active = false;
    }

    public boolean isActive() { return active; }
    public int     getTileX() { return TILE_X; }
    public int     getTileY() { return TILE_Y; }
    public int     getPoints(){ return type.points; }

    public void collect() { active = false; }

    public void draw(Graphics2D g2) {
        if (!active) return;
        int T  = GamePanel.TILE_SIZE;
        int px = TILE_X * T;
        int py = TILE_Y * T;
        // Pisca nos últimos 3 segundos
        if (timer <= 60 * 3 && (timer / 10) % 2 == 0) return;

        switch (type) {
            case CHERRY     -> drawCherry(g2, px, py, T);
            case STRAWBERRY -> drawStrawberry(g2, px, py, T);
            case ORANGE     -> drawOrange(g2, px, py, T);
            case APPLE      -> drawApple(g2, px, py, T);
            case MELON      -> drawMelon(g2, px, py, T);
            case GALAXIAN   -> drawGalaxian(g2, px, py, T);
            case BELL       -> drawBell(g2, px, py, T);
            case KEY        -> drawKey(g2, px, py, T);
        }
    }

    // ---- Sprites individuais ----

    private void drawCherry(Graphics2D g2, int px, int py, int T) {
        // Cabo verde
        g2.setColor(new Color(0, 200, 0));
        g2.setStroke(new BasicStroke(2));
        g2.drawArc(px + T/2 - 4, py, 10, 10, 0, 180);
        g2.drawArc(px + T/2 - 2, py, 10, 10, 0, 180);
        g2.setStroke(new BasicStroke(1));
        // Cerejas
        g2.setColor(Color.RED);
        g2.fillOval(px + 2, py + T/3, T/3, T/3);
        g2.fillOval(px + T/2 + 1, py + T/3, T/3, T/3);
        // Brilho
        g2.setColor(new Color(255, 255, 255, 140));
        g2.fillOval(px + 3, py + T/3 + 1, 4, 4);
        g2.fillOval(px + T/2 + 2, py + T/3 + 1, 4, 4);
    }

    private void drawStrawberry(Graphics2D g2, int px, int py, int T) {
        int cx = px + T/2, cy = py + T/2 + 2;
        g2.setColor(Color.RED);
        Path2D body = new Path2D.Double();
        body.moveTo(cx, cy + T/3);
        body.curveTo(cx - T/3, cy + T/3, cx - T/2, cy, cx - T/3, cy - T/4);
        body.curveTo(cx - T/4, cy - T/2, cx, cy - T/3, cx, cy - T/3);
        body.curveTo(cx, cy - T/3, cx + T/4, cy - T/2, cx + T/3, cy - T/4);
        body.curveTo(cx + T/2, cy, cx + T/3, cy + T/3, cx, cy + T/3);
        body.closePath();
        g2.fill(body);
        // Folha
        g2.setColor(new Color(0, 180, 0));
        g2.fillOval(cx - 4, py + 2, 8, 7);
        // Sementes
        g2.setColor(Color.YELLOW);
        g2.fillOval(cx - 5, cy - 3, 2, 2);
        g2.fillOval(cx + 2, cy - 3, 2, 2);
        g2.fillOval(cx - 3, cy + 2, 2, 2);
    }

    private void drawOrange(Graphics2D g2, int px, int py, int T) {
        int cx = px + T/2, cy = py + T/2 + 2;
        g2.setColor(new Color(255, 140, 0));
        g2.fillOval(cx - T/3, cy - T/3, T*2/3, T*2/3);
        // Cabo
        g2.setColor(new Color(0, 180, 0));
        g2.setStroke(new BasicStroke(2));
        g2.drawLine(cx, cy - T/3, cx, cy - T/3 - 6);
        g2.setStroke(new BasicStroke(1));
    }

    private void drawApple(Graphics2D g2, int px, int py, int T) {
        int cx = px + T/2;
        g2.setColor(Color.RED);
        Path2D body = new Path2D.Double();
        body.moveTo(cx, py + T*2/3);
        body.curveTo(cx - T/2, py + T*2/3, cx - T/2, py + T/3, cx - T/4, py + T/4);
        body.curveTo(cx - T/8, py + T/5, cx, py + T/3, cx, py + T/3);
        body.curveTo(cx, py + T/3, cx + T/8, py + T/5, cx + T/4, py + T/4);
        body.curveTo(cx + T/2, py + T/3, cx + T/2, py + T*2/3, cx, py + T*2/3);
        body.closePath();
        g2.fill(body);
        // Cabo marrom
        g2.setColor(new Color(92, 64, 51));
        g2.fillRect(cx - 2, py + 3, 3, 9);
    }

    private void drawMelon(Graphics2D g2, int px, int py, int T) {
        int cx = px + T/2, cy = py + T/2 + 2;
        g2.setColor(new Color(0, 220, 0));
        g2.fillOval(cx - T/3, cy - T/3, T*2/3, T*2/3);
        g2.setColor(new Color(0, 130, 0));
        g2.setStroke(new BasicStroke(1.5f));
        for (int i = -1; i <= 1; i++) {
            int oy = i * (T/6);
            g2.drawArc(cx - T/3, cy - T/3 + oy, T*2/3, T/3, 0, 180);
        }
        g2.setStroke(new BasicStroke(1));
    }

    private void drawGalaxian(Graphics2D g2, int px, int py, int T) {
        int cx = px + T/2;
        // Asa principal amarela
        g2.setColor(Color.YELLOW);
        int[] xp = { cx, cx - T/4, cx - T/4, cx - T/8, cx - T/8, cx + T/8, cx + T/8, cx + T/4, cx + T/4 };
        int[] yp = { py + 4, py + T/3, py + T/2, py + T/2, py + T*2/3, py + T*2/3, py + T/2, py + T/2, py + T/3 };
        g2.fillPolygon(xp, yp, xp.length);
        // Núcleo azul
        g2.setColor(Color.BLUE);
        g2.fillRect(cx - 4, py + T/3, 8, T/4);
        // Faixa vermelha
        g2.setColor(Color.RED);
        g2.fillRect(cx - T/4, py + T/2 - 2, T/2, 4);
    }

    private void drawBell(Graphics2D g2, int px, int py, int T) {
        int cx = px + T/2;
        g2.setColor(Color.YELLOW);
        Path2D body = new Path2D.Double();
        body.moveTo(cx, py + 4);
        body.curveTo(cx - T/3, py + 4, cx - T/3, py + T*2/3, cx - T/3, py + T*2/3);
        body.lineTo(cx + T/3, py + T*2/3);
        body.curveTo(cx + T/3, py + 4, cx + T/3, py + 4, cx, py + 4);
        body.closePath();
        g2.fill(body);
        // Base
        g2.fillRoundRect(px + 3, py + T*2/3 - 1, T - 6, 5, 3, 3);
        // Badalo branco
        g2.setColor(Color.WHITE);
        g2.fillOval(cx - 4, py + T*2/3 + 2, 8, 8);
    }

    private void drawKey(Graphics2D g2, int px, int py, int T) {
        int cx = px + T/2;
        g2.setColor(Color.CYAN);
        g2.setStroke(new BasicStroke(3, BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND));
        // Anel da chave
        g2.drawOval(cx - 6, py + 3, 12, 12);
        // Haste
        g2.drawLine(cx, py + 15, cx, py + T - 3);
        // Dentes
        g2.drawLine(cx, py + T - 7,  cx + 5, py + T - 7);
        g2.drawLine(cx, py + T - 12, cx + 5, py + T - 12);
        g2.setStroke(new BasicStroke(1));
    }
}
