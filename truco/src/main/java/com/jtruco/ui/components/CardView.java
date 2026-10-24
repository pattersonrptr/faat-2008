package com.jtruco.ui.components;

import com.jtruco.core.card.Card;
import com.jtruco.core.card.Suit;
import com.jtruco.core.game.TrucoVariant;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Label;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.StackPane;
import javafx.scene.shape.Rectangle;

public class CardView extends StackPane {

    public static final double W = 68;
    public static final double H = 98;

    /** Face-down card (for AI players). */
    public CardView() {
        getStyleClass().add("card-back");
        Rectangle inner = new Rectangle(W - 12, H - 12);
        inner.setArcWidth(6);
        inner.setArcHeight(6);
        inner.setStyle("-fx-fill: rgba(255,255,255,0.05); -fx-stroke: rgba(255,255,255,0.15); -fx-stroke-width: 1;");
        getChildren().add(inner);
    }

    /** Face-up card, not clickable. */
    public CardView(Card card, TrucoVariant variant, Card vira) {
        this(card, variant, vira, null);
    }

    /** Face-up card, optionally clickable. */
    public CardView(Card card, TrucoVariant variant, Card vira, Runnable onClick) {
        getStyleClass().add("card");

        boolean isManilha = isManilha(card, variant, vira);
        if (isManilha) getStyleClass().add("card-manilha");

        if (onClick != null) {
            getStyleClass().add("card-clickable");
            setOnMouseClicked(e -> onClick.run());
        }

        String colorClass = isRed(card.getSuit()) ? "red" : "black";

        // Top-left: rank
        Label topRank = new Label(card.getRank().getLabel());
        topRank.getStyleClass().addAll("card-rank", colorClass);

        // Bottom-right: rank (upside-down via rotation)
        Label botRank = new Label(card.getRank().getLabel());
        botRank.getStyleClass().addAll("card-rank", colorClass);
        botRank.setRotate(180);

        // Center: suit symbol
        Label suitLabel = new Label(card.getSuit().getSymbol());
        suitLabel.getStyleClass().addAll("card-suit", colorClass);

        // Use AnchorPane for precise corner placement
        AnchorPane anchor = new AnchorPane(topRank, botRank, suitLabel);
        anchor.setMinSize(W, H);
        anchor.setMaxSize(W, H);

        AnchorPane.setTopAnchor(topRank,  5.0);
        AnchorPane.setLeftAnchor(topRank, 6.0);
        AnchorPane.setBottomAnchor(botRank,  5.0);
        AnchorPane.setRightAnchor(botRank,   6.0);
        AnchorPane.setTopAnchor(suitLabel,   (H - 30) / 2.0);
        AnchorPane.setLeftAnchor(suitLabel,  (W - 22) / 2.0);

        setMinSize(W, H);
        setMaxSize(W, H);
        setPadding(Insets.EMPTY);
        getChildren().add(anchor);
    }

    private static boolean isRed(Suit suit) {
        return suit == Suit.COPAS || suit == Suit.OUROS;
    }

    private static boolean isManilha(Card card, TrucoVariant variant, Card vira) {
        if (variant == null) return false;
        int strength = variant.cardStrength(card, vira);
        return strength >= 11; // manilha threshold covers Zap(14),Copas(13),Espadilha(12),Ouros(11)
    }
}
