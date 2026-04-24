package com.jtruco.core.team;

import com.jtruco.core.player.Player;

import java.util.List;

public class Team {

    private final String name;
    private final List<Player> players;
    private int score;

    public Team(String name, Player... players) {
        this.name = name;
        this.players = List.of(players);
        this.score = 0;
    }

    public String getName() { return name; }
    public List<Player> getPlayers() { return players; }
    public int getScore() { return score; }

    public void addPoints(int points) {
        score += points;
    }

    public boolean contains(Player player) {
        return players.contains(player);
    }

    public void resetScore() {
        score = 0;
    }
}
