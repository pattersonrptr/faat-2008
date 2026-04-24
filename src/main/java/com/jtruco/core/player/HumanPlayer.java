package com.jtruco.core.player;

public class HumanPlayer extends Player {

    public HumanPlayer(String name) {
        super(name);
    }

    @Override
    public boolean isHuman() { return true; }
}
