package com.jtruco.core.game;

import com.jtruco.core.card.Card;
import com.jtruco.core.player.Player;
import com.jtruco.core.team.Team;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public class Round {

    private final TrucoVariant variant;
    private final List<Player> players;
    private final List<Team> teams;
    private final Card vira;
    private final List<Trick> tricks = new ArrayList<>(3);

    private int currentBidPoints;
    private boolean finished;

    /** Index into players list of whoever leads the current trick. */
    private int leadPlayerIndex;
    /** How many plays have been made so far in the current trick. */
    private int playsInCurrentTrick;

    public Round(TrucoVariant variant, List<Player> players, List<Team> teams, Card vira, int leadPlayerIndex) {
        this.variant = variant;
        this.players = players;
        this.teams = teams;
        this.vira = vira;
        this.currentBidPoints = 1;
        this.finished = false;
        this.leadPlayerIndex = leadPlayerIndex;
        this.playsInCurrentTrick = 0;
        tricks.add(new Trick(variant, vira));
    }

    /** The player who must act next. */
    public Player getCurrentPlayer() {
        int idx = (leadPlayerIndex + playsInCurrentTrick) % players.size();
        return players.get(idx);
    }

    public Trick currentTrick() {
        return tricks.getLast();
    }

    public void playCard(Player player, Card card) {
        if (!player.equals(getCurrentPlayer())) {
            throw new IllegalStateException("It is not " + player.getName() + "'s turn.");
        }
        player.removeCard(card);
        currentTrick().addPlay(player, card);
        playsInCurrentTrick++;

        if (currentTrick().isComplete(players.size())) {
            advanceTrick();
        }
    }

    private void advanceTrick() {
        Optional<Player> winner = currentTrick().getWinner();
        if (winner.isPresent()) {
            leadPlayerIndex = players.indexOf(winner.get());
        }
        // On a tie, the lead stays with the same player (leadPlayerIndex unchanged).
        playsInCurrentTrick = 0;

        if (!isRoundOver()) {
            tricks.add(new Trick(variant, vira));
        } else {
            finished = true;
        }
    }

    public boolean isRoundOver() {
        long completedTricks = tricks.stream()
                .filter(t -> t.isComplete(players.size()))
                .count();

        if (completedTricks < 2) return false;

        // Check if a team has already clinched 2 trick wins
        return determineRoundWinner().isPresent() || completedTricks == 3;
    }

    public Optional<Team> determineRoundWinner() {
        List<Optional<Player>> trickWinners = tricks.stream()
                .filter(t -> t.isComplete(players.size()))
                .map(Trick::getWinner)
                .toList();

        long team0wins = countWins(trickWinners, teams.get(0));
        long team1wins = countWins(trickWinners, teams.get(1));

        if (team0wins >= 2) return Optional.of(teams.get(0));
        if (team1wins >= 2) return Optional.of(teams.get(1));

        // All 3 tricks done: first trick winner breaks the tie
        if (trickWinners.size() == 3) {
            Optional<Player> firstWinner = trickWinners.get(0);
            if (firstWinner.isPresent()) {
                return teams.stream().filter(t -> t.contains(firstWinner.get())).findFirst();
            }
            // All three tied: no winner (extremely rare — treat as team0 win per Paulista rules)
            return Optional.of(teams.get(0));
        }

        return Optional.empty();
    }

    private long countWins(List<Optional<Player>> winners, Team team) {
        return winners.stream()
                .filter(Optional::isPresent)
                .map(Optional::get)
                .filter(team::contains)
                .count();
    }

    public int getCurrentBidPoints() { return currentBidPoints; }
    public void setCurrentBidPoints(int points) { this.currentBidPoints = points; }

    public Card getVira() { return vira; }
    public List<Trick> getTricks() { return List.copyOf(tricks); }
    public List<Player> getPlayers() { return players; }
    public boolean isFinished() { return finished; }
    public int getLeadPlayerIndex() { return leadPlayerIndex; }
}
