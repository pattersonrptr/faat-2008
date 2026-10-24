package com.jtruco.core.game;

import com.jtruco.core.card.Card;
import com.jtruco.core.player.Player;
import com.jtruco.core.team.Team;

import java.util.List;
import java.util.Optional;

/**
 * Read-only view of the game state exposed to AI strategies.
 * Contains everything an AI player can legitimately observe.
 */
public class GameContext {

    private final Player self;
    private final Team myTeam;
    private final Team opponentTeam;
    private final Round round;
    private final Match match;

    public GameContext(Player self, Team myTeam, Team opponentTeam, Round round, Match match) {
        this.self = self;
        this.myTeam = myTeam;
        this.opponentTeam = opponentTeam;
        this.round = round;
        this.match = match;
    }

    public Player getSelf() { return self; }
    public Team getMyTeam() { return myTeam; }
    public Team getOpponentTeam() { return opponentTeam; }
    public Round getRound() { return round; }
    public Match getMatch() { return match; }
    public TrucoVariant getVariant() { return match.getVariant(); }

    public List<Card> getMyHand() {
        return self.getHand();
    }

    /** The partner of this AI player (null if no partner, e.g. 1v1). */
    public Optional<Player> getPartner() {
        return myTeam.getPlayers().stream()
                .filter(p -> !p.equals(self))
                .findFirst();
    }

    /**
     * True if my team's partner has already played in the current trick
     * and is currently winning it.
     */
    public boolean isPartnerWinningCurrentTrick() {
        Trick trick = round.currentTrick();
        List<Trick.Play> plays = trick.getPlays();
        if (plays.isEmpty()) return false;

        Optional<Player> currentBest = trick.getWinner();
        return currentBest.isPresent() && myTeam.contains(currentBest.get())
                && !currentBest.get().equals(self);
    }

    /** The strongest card played so far in the current trick, if any. */
    public Optional<Card> strongestCardInCurrentTrick() {
        Trick trick = round.currentTrick();
        return trick.getPlays().stream()
                .map(Trick.Play::card)
                .max((a, b) -> getVariant().cardStrength(a, round.getVira())
                        - getVariant().cardStrength(b, round.getVira()));
    }

    /** Compute the total strength of the given hand (used for bid/call decisions). */
    public int handStrength(List<Card> hand) {
        return hand.stream()
                .mapToInt(c -> getVariant().cardStrength(c, round.getVira()))
                .sum();
    }

    public int myHandStrength() {
        return handStrength(getMyHand());
    }

    public int myScore() { return myTeam.getScore(); }
    public int opponentScore() { return opponentTeam.getScore(); }
    public int winningScore() { return getVariant().getWinningScore(); }
}
