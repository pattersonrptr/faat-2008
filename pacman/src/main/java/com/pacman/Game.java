package com.pacman;

import javax.swing.JFrame;

public class Game extends JFrame {

    public Game() {
        setTitle("Pac-Man");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setResizable(false);

        GamePanel panel = new GamePanel();
        add(panel);
        pack();

        setLocationRelativeTo(null);
        setVisible(true);

        panel.start();
    }
}
