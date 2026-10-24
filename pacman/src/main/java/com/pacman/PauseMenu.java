package com.pacman;

import java.awt.*;
import java.awt.event.*;

/**
 * Menu de pausa sobreposto ao jogo.
 * Itens: Continuar | Reiniciar | Música (vol) | Efeitos (vol) | Sair
 *
 * Navegação: setas cima/baixo para selecionar, Enter para confirmar,
 * setas esquerda/direita para ajustar volume.
 */
public class PauseMenu {

    public enum Action { NONE, RESUME, RESTART, QUIT }

    private static final String[] LABELS = {
        "Continuar", "Reiniciar", "Música", "Efeitos", "Sair"
    };
    private static final int IDX_RESUME  = 0;
    private static final int IDX_RESTART = 1;
    private static final int IDX_MUSIC   = 2;
    private static final int IDX_EFFECT  = 3;
    private static final int IDX_QUIT    = 4;

    private int selectedIndex = 0;
    private final SoundManager sound;

    public PauseMenu(SoundManager sound) {
        this.sound = sound;
    }

    public void resetSelection() { selectedIndex = 0; }

    /**
     * Processa uma tecla e retorna a ação resultante.
     */
    public Action handleKey(int keyCode) {
        switch (keyCode) {
            case KeyEvent.VK_UP -> {
                selectedIndex = (selectedIndex - 1 + LABELS.length) % LABELS.length;
                return Action.NONE;
            }
            case KeyEvent.VK_DOWN -> {
                selectedIndex = (selectedIndex + 1) % LABELS.length;
                return Action.NONE;
            }
            case KeyEvent.VK_LEFT  -> { adjustSelected(-0.1f); return Action.NONE; }
            case KeyEvent.VK_RIGHT -> { adjustSelected(+0.1f); return Action.NONE; }
            case KeyEvent.VK_ENTER -> {
                return switch (selectedIndex) {
                    case IDX_RESUME  -> Action.RESUME;
                    case IDX_RESTART -> Action.RESTART;
                    case IDX_QUIT    -> Action.QUIT;
                    default          -> Action.NONE;
                };
            }
            case KeyEvent.VK_M -> {
                if (selectedIndex == IDX_MUSIC) {
                    sound.setMusicMuted(!sound.isMusicMuted());
                }
                return Action.NONE;
            }
            case KeyEvent.VK_E -> {
                if (selectedIndex == IDX_EFFECT) {
                    sound.setEffectMuted(!sound.isEffectMuted());
                }
                return Action.NONE;
            }
        }
        return Action.NONE;
    }

    private void adjustSelected(float delta) {
        if (selectedIndex == IDX_MUSIC) {
            sound.setMusicVolume(sound.getMusicVolume() + delta);
        } else if (selectedIndex == IDX_EFFECT) {
            sound.setEffectVolume(sound.getEffectVolume() + delta);
        }
    }

    public void draw(Graphics2D g2, int panelW, int panelH) {
        // Fundo semitransparente
        g2.setColor(new Color(0, 0, 0, 200));
        g2.fillRect(0, 0, panelW, panelH);

        int boxW = 260, boxH = 280;
        int boxX = (panelW - boxW) / 2;
        int boxY = (panelH - boxH) / 2;

        // Caixa
        g2.setColor(new Color(20, 20, 80));
        g2.fillRoundRect(boxX, boxY, boxW, boxH, 16, 16);
        g2.setColor(new Color(33, 33, 255));
        g2.setStroke(new BasicStroke(2));
        g2.drawRoundRect(boxX, boxY, boxW, boxH, 16, 16);
        g2.setStroke(new BasicStroke(1));

        // Título
        g2.setColor(Color.YELLOW);
        g2.setFont(new Font("Arial", Font.BOLD, 22));
        FontMetrics fm = g2.getFontMetrics();
        String title = "PAUSADO";
        g2.drawString(title, boxX + (boxW - fm.stringWidth(title)) / 2, boxY + 38);

        // Itens
        int itemY = boxY + 75;
        for (int i = 0; i < LABELS.length; i++) {
            boolean sel = (i == selectedIndex);

            if (sel) {
                g2.setColor(new Color(255, 255, 100, 60));
                g2.fillRoundRect(boxX + 12, itemY - 18, boxW - 24, 26, 8, 8);
            }

            g2.setColor(sel ? Color.YELLOW : Color.WHITE);
            g2.setFont(new Font("Arial", sel ? Font.BOLD : Font.PLAIN, 16));
            fm = g2.getFontMetrics();

            String label;
            if (i == IDX_MUSIC) {
                String muted = sound.isMusicMuted() ? " [MUDO]" : "";
                label = "Música" + muted + "  " + volumeBar(sound.getMusicVolume());
            } else if (i == IDX_EFFECT) {
                String muted = sound.isEffectMuted() ? " [MUDO]" : "";
                label = "Efeitos" + muted + "  " + volumeBar(sound.getEffectVolume());
            } else {
                label = LABELS[i];
            }

            g2.drawString(label, boxX + 20, itemY);

            // Indicador selecionado
            if (sel) {
                g2.setColor(Color.YELLOW);
                g2.drawString("▶", boxX + 4, itemY);
            }

            itemY += 38;
        }

        // Rodapé de ajuda
        g2.setColor(new Color(180, 180, 180));
        g2.setFont(new Font("Arial", Font.PLAIN, 11));
        String hint1 = "↑↓ Navegar  ←→ Ajustar volume";
        String hint2 = "M/E Mutar   Enter Confirmar";
        fm = g2.getFontMetrics();
        g2.drawString(hint1, boxX + (boxW - fm.stringWidth(hint1)) / 2, boxY + boxH - 24);
        g2.drawString(hint2, boxX + (boxW - fm.stringWidth(hint2)) / 2, boxY + boxH - 10);
    }

    /** Barra de volume textual: ████░░░░ */
    private String volumeBar(float vol) {
        int filled = Math.round(vol * 8);
        return "█".repeat(filled) + "░".repeat(8 - filled);
    }
}
