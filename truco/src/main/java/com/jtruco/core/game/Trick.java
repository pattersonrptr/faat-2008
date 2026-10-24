package com.jtruco.core.game;

import com.jtruco.core.card.Card;
import com.jtruco.core.player.Player;

import java.util.*;

public class Trick {

    public record Play(Player player, Card card) {}

    private final List<Play> plays = new ArrayList<>(4);
    private final TrucoVariant variant;
    private final Card vira;

    public Trick(TrucoVariant variant, Card vira) {
        this.variant = variant;
        this.vira = vira;
    }

    public void addPlay(Player player, Card card) {
        plays.add(new Play(player, card));
    }

    public List<Play> getPlays() {
        return Collections.unmodifiableList(plays);
    }

    public int getPlayCount() {
        return plays.size();
    }

    /**
     * Returns the winning player, or empty if tied.
     */
    public Optional<Player> getWinner() {
        if (plays.isEmpty()) return Optional.empty();

        Play best = plays.getFirst();
        boolean tied = false;

        for (int i = 1; i < plays.size(); i++) {
            Play current = plays.get(i);
            int cmp = variant.cardStrength(current.card(), vira)
                    - variant.cardStrength(best.card(), vira);
            if (cmp > 0) {
                best = current;
                tied = false;
            } else if (cmp == 0) {
                tied = true;
            }
        }

        return tied ? Optional.empty() : Optional.of(best.player());
    }

    public boolean isComplete(int playerCount) {
        return plays.size() == playerCount;
    }
}
