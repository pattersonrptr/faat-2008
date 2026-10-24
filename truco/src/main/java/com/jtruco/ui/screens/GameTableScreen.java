package com.jtruco.ui.screens;

import com.jtruco.core.card.Card;
import com.jtruco.core.game.*;
import com.jtruco.core.player.Player;
import com.jtruco.core.team.Team;
import com.jtruco.ui.components.*;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Label;
import javafx.scene.layout.*;

import java.util.List;

/**
 * The main game table.
 *
 * Player seating (matches players list order):
 *   players[0] = Human     → bottom-center
 *   players[1] = OpponentA → left-center
 *   players[2] = Partner   → top-center
 *   players[3] = OpponentB → right-center
 */
public class GameTableScreen extends BorderPane {

    private final GameController controller;
    private final TrucoVariant   variant;
    private final List<Player>   players;
    private final List<Team>     teams;

    private final ScoreBar  scoreBar;
    private final ActionBar actionBar;
    private final TrickArea trickArea;

    // Card rows for each player (indexed 0-3 matching players list)
    private final HBox[] handBoxes = new HBox[4];
    private final Label[] trickCountLabels = new Label[4];

    public GameTableScreen(GameController controller) {
        this.controller = controller;
        this.variant    = controller.getMatch().getVariant();
        this.players    = controller.getMatch().getPlayers();
        this.teams      = controller.getMatch().getTeams();

        scoreBar  = new ScoreBar(teams, variant.getName());
        actionBar = new ActionBar();

        // Trick area placeholder (will be reset on each round start)
        trickArea = new TrickArea(variant, null);

        setTop(scoreBar);
        setCenter(buildTable());
        setBottom(actionBar);

        wireActionBar();
        controller.addListener(this::onEvent);

        // Start the first round
        controller.startRound();
    }

    // ── Table layout ─────────────────────────────────────────────────────────

    private Pane buildTable() {
        // Outer padding around the felt
        StackPane wrapper = new StackPane();
        wrapper.setPadding(new Insets(16));
        wrapper.setStyle("-fx-background-color: #1a1a2e;");

        // Green felt
        AnchorPane felt = new AnchorPane();
        felt.setId("tableFelt");

        // --- Partner (top-center) ---
        VBox partnerArea = playerArea(2, false);
        AnchorPane.setTopAnchor(partnerArea,    16.0);
        AnchorPane.setLeftAnchor(partnerArea,    0.0);
        AnchorPane.setRightAnchor(partnerArea,   0.0);

        // --- OpponentA (left-center) ---
        VBox oppAArea = playerArea(1, false);
        oppAArea.setRotate(-90);
        AnchorPane.setTopAnchor(oppAArea,       0.0);
        AnchorPane.setBottomAnchor(oppAArea,    0.0);
        AnchorPane.setLeftAnchor(oppAArea,     -60.0); // rotated, so shift left

        // --- OpponentB (right-center) ---
        VBox oppBArea = playerArea(3, false);
        oppBArea.setRotate(90);
        AnchorPane.setTopAnchor(oppBArea,       0.0);
        AnchorPane.setBottomAnchor(oppBArea,    0.0);
        AnchorPane.setRightAnchor(oppBArea,    -60.0); // rotated, so shift right

        // --- Human (bottom-center) ---
        VBox humanArea = playerArea(0, true);
        AnchorPane.setBottomAnchor(humanArea,   16.0);
        AnchorPane.setLeftAnchor(humanArea,      0.0);
        AnchorPane.setRightAnchor(humanArea,     0.0);

        // --- Trick area (center) ---
        StackPane trickWrapper = new StackPane(trickArea);
        trickWrapper.setAlignment(Pos.CENTER);
        AnchorPane.setTopAnchor(trickWrapper,    0.0);
        AnchorPane.setBottomAnchor(trickWrapper, 0.0);
        AnchorPane.setLeftAnchor(trickWrapper,   0.0);
        AnchorPane.setRightAnchor(trickWrapper,  0.0);

        // --- Vira card (top-right corner) ---
        VBox viraBox = new VBox(4);
        viraBox.setId("viraBox");
        viraBox.setAlignment(Pos.CENTER);
        Label viraLabel = new Label("Vira");
        viraLabel.getStyleClass().add("vira-label");
        viraBox.getChildren().addAll(viraLabel, new StackPane()); // placeholder, filled on round start
        AnchorPane.setTopAnchor(viraBox,   12.0);
        AnchorPane.setRightAnchor(viraBox, 12.0);

        felt.getChildren().addAll(trickWrapper, partnerArea, humanArea, oppAArea, oppBArea, viraBox);

        wrapper.getChildren().add(felt);
        StackPane.setAlignment(felt, Pos.CENTER);
        felt.prefWidthProperty().bind(wrapper.widthProperty().subtract(32));
        felt.prefHeightProperty().bind(wrapper.heightProperty().subtract(32));

        return wrapper;
    }

    private VBox playerArea(int seatIndex, boolean isHuman) {
        Player player = players.get(seatIndex);
        Label nameLabel = new Label(player.getName());
        nameLabel.getStyleClass().add(isHuman ? "player-label-human" : "player-label");

        Label trickCount = new Label("");
        trickCount.getStyleClass().add("variant-label");
        trickCountLabels[seatIndex] = trickCount;

        HBox handBox = new HBox(8);
        handBox.setAlignment(Pos.CENTER);
        handBoxes[seatIndex] = handBox;

        VBox area = new VBox(6);
        area.setAlignment(Pos.CENTER);

        if (isHuman) {
            area.getChildren().addAll(trickCount, nameLabel, handBox);
        } else {
            area.getChildren().addAll(handBox, nameLabel, trickCount);
        }
        return area;
    }

    // ── Round/hand rendering ─────────────────────────────────────────────────

    private void refreshHands() {
        Round round = controller.getMatch().getCurrentRound();

        for (int i = 0; i < players.size(); i++) {
            Player player  = players.get(i);
            HBox   handBox = handBoxes[i];
            handBox.getChildren().clear();

            List<Card> hand = player.getHand();
            boolean isHuman = player.isHuman();

            for (Card card : hand) {
                CardView cv;
                if (isHuman && controller.isHumanTurn()) {
                    cv = new CardView(card, variant, round.getVira(), () -> onHumanCardClicked(card));
                } else if (isHuman) {
                    cv = new CardView(card, variant, round.getVira()); // face-up but not clickable
                } else {
                    cv = new CardView(); // face-down
                }
                handBox.getChildren().add(cv);
            }
        }
    }

    private void refreshVira(Round round) {
        AnchorPane felt = (AnchorPane) ((StackPane) getCenter()).getChildren().getFirst();
        VBox viraBox = (VBox) felt.getChildren().get(felt.getChildren().size() - 1);
        viraBox.getChildren().setAll(
            viraBox.getChildren().getFirst(), // keep the "Vira" label
            new CardView(round.getVira(), variant, null)
        );
    }

    private void refreshScores() {
        Team my  = teams.get(0);
        Team opp = teams.get(1);
        scoreBar.update(my.getScore(), opp.getScore());
    }

    // ── Event handling ───────────────────────────────────────────────────────

    private void onEvent(GameController.GameEvent event) {
        // All events fire on the FX thread (game logic runs synchronously within FX handlers)
        switch (event.type()) {
            case ROUND_STARTED -> {
                Round round = controller.getMatch().getCurrentRound();
                trickArea.refresh(round);
                refreshHands();
                refreshVira(round);
                refreshScores();
                actionBar.setMode(ActionBar.Mode.PLAY, "Sua vez — clique em uma carta.", false);
            }
            case CARD_PLAYED -> {
                Round round = controller.getMatch().getCurrentRound();
                trickArea.refresh(round);
                refreshHands();
            }
            case TRICK_WON, TRICK_TIED -> {
                actionBar.setMessage(event.message());
                if (!controller.getMatch().getCurrentRound().isFinished()) {
                    // Clear trick area for next trick after a brief visual pause
                    // (no Thread.sleep — just clear immediately; UI already showed the play)
                    trickArea.refresh(controller.getMatch().getCurrentRound());
                }
            }
            case ROUND_WON -> {
                refreshScores();
                String msg = event.message() + " | Placar: "
                        + teams.get(0).getScore() + " × " + teams.get(1).getScore();
                if (controller.getMatch().isOver()) {
                    actionBar.setMode(ActionBar.Mode.MATCH_OVER, msg + " — Jogo encerrado!", false);
                } else {
                    actionBar.setMode(ActionBar.Mode.ROUND_OVER, msg, false);
                }
            }
            case TRUCO_CALLED ->
                actionBar.setMessage(event.message());
            case BID_DECISION_REQUIRED -> {
                BidCall call = (BidCall) event.data();
                boolean canRaise = variant.getTrucoBidSequence().stream()
                        .anyMatch(b -> b.pointsIfAccepted() > call.pointsIfAccepted());
                actionBar.setMode(ActionBar.Mode.BID_RESPONSE, event.message(), canRaise);
            }
            case BID_ACCEPTED, BID_REJECTED ->
                actionBar.setMessage(event.message());
            case MATCH_WON -> {
                refreshScores();
                actionBar.setMode(ActionBar.Mode.MATCH_OVER, "🏆 " + event.message(), false);
            }
        }
    }

    // ── User interactions ─────────────────────────────────────────────────────

    private void onHumanCardClicked(Card card) {
        if (!controller.isHumanTurn()) return;
        try {
            controller.humanPlayCard(card);
        } catch (Exception ex) {
            actionBar.setMessage("Erro: " + ex.getMessage());
        }
    }

    private void wireActionBar() {
        actionBar.onTruco(() -> {
            controller.humanCallTruco();
            actionBar.setMessage("Você pediu Truco!");
        });

        actionBar.onAccept(() -> {
            controller.humanRespondToBid(BidResponse.ACCEPT);
            actionBar.setMode(ActionBar.Mode.PLAY, "Truco aceito — continue jogando.", false);
            refreshHands();
        });

        actionBar.onRaise(() -> {
            controller.humanRespondToBid(BidResponse.RAISE);
            actionBar.setMode(ActionBar.Mode.PLAY, "Você aumentou!", false);
            refreshHands();
        });

        actionBar.onRun(() -> {
            controller.humanRespondToBid(BidResponse.REJECT);
            refreshScores();
        });

        actionBar.onNextRound(() -> {
            if (!controller.getMatch().isOver()) {
                controller.startRound();
            }
        });
    }
}
