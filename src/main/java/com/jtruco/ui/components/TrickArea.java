package com.jtruco.ui.components;

import com.jtruco.core.card.Card;
import com.jtruco.core.game.GameController;
import com.jtruco.core.game.Round;
import com.jtruco.core.game.Trick;
import com.jtruco.core.game.TrucoVariant;
import com.jtruco.core.player.Player;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.StackPane;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Shows the four play slots for the current trick arranged in a compass:
 *   Top    = Partner   (players[2])
 *   Left   = OpponentA (players[1])
 *   Right  = OpponentB (players[3])
 *   Bottom = Human     (players[0])
 */
public class TrickArea extends GridPane {

    // Grid positions [row, col] for each player seat
    private static final int[][] SEAT_GRID = {
        {1, 1},  // 0 = Human     → center-bottom row
        {1, 0},  // 1 = OpponentA → center-left col
        {0, 1},  // 2 = Partner   → top-center
        {1, 2},  // 3 = OpponentB → center-right col
    };

    private final StackPane[] slots = new StackPane[4];
    private TrucoVariant variant;
    private Card vira;

    public TrickArea(TrucoVariant variant, Card vira) {
        this.variant = variant;
        this.vira = vira;

        setHgap(12);
        setVgap(12);
        setAlignment(Pos.CENTER);
        setPadding(new Insets(8));

        for (int i = 0; i < 4; i++) {
            StackPane slot = new StackPane();
            slot.getStyleClass().add("trick-slot");
            slot.setMinSize(CardView.W, CardView.H);
            slot.setMaxSize(CardView.W, CardView.H);
            int[] pos = SEAT_GRID[i];
            add(slot, pos[1], pos[0]);
            slots[i] = slot;
        }
    }

    /** Refresh display from the current round state. */
    public void refresh(Round round) {
        this.vira = round.getVira();
        List<Player> players = round.getPlayers();

        // Clear all slots
        for (StackPane slot : slots) slot.getChildren().clear();

        // Find cards played in the current (incomplete) trick
        Trick current = round.currentTrick();
        Map<Player, Card> played = new HashMap<>();
        for (Trick.Play play : current.getPlays()) {
            played.put(play.player(), play.card());
        }

        for (int i = 0; i < players.size(); i++) {
            Card card = played.get(players.get(i));
            if (card != null) {
                slots[i].getChildren().add(new CardView(card, variant, vira));
            }
        }
    }

    public void clear() {
        for (StackPane slot : slots) slot.getChildren().clear();
    }
}
