package com.jtruco.core.game;

import com.jtruco.core.card.Card;
import com.jtruco.core.card.Deck;
import com.jtruco.core.player.Player;
import com.jtruco.core.team.Team;

import java.util.List;
import java.util.Optional;

public class Match {

    private final TrucoVariant variant;
    private final List<Player> players;
    private final List<Team> teams;

    private Round currentRound;
    /** Index of the current dealer; the player after the dealer leads each round. */
    private int dealerIndex;

    public Match(TrucoVariant variant, List<Player> players, List<Team> teams) {
        this.variant = variant;
        this.players = List.copyOf(players);
        this.teams = List.copyOf(teams);
        this.dealerIndex = 0;
    }

    public void startNextRound() {
        Deck deck = new Deck();
        deck.shuffle();

        for (Player player : players) {
            player.clearHand();
            for (int i = 0; i < 3; i++) {
                player.receiveCard(deck.deal());
            }
        }

        Card vira = deck.deal();
        int leadIndex = (dealerIndex + 1) % players.size();
        currentRound = new Round(variant, players, teams, vira, leadIndex);
        dealerIndex = (dealerIndex + 1) % players.size();
    }

    public void applyRoundResult() {
        Optional<Team> winner = currentRound.determineRoundWinner();
        winner.ifPresent(team -> team.addPoints(currentRound.getCurrentBidPoints()));
    }

    public Optional<Team> getMatchWinner() {
        return teams.stream()
                .filter(t -> t.getScore() >= variant.getWinningScore())
                .findFirst();
    }

    public boolean isOver() {
        return getMatchWinner().isPresent();
    }

    public Round getCurrentRound() { return currentRound; }
    public List<Player> getPlayers() { return players; }
    public List<Team> getTeams() { return teams; }
    public TrucoVariant getVariant() { return variant; }
}
