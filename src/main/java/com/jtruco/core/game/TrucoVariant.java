package com.jtruco.core.game;

import com.jtruco.core.card.Card;
import com.jtruco.core.player.Player;

import java.util.List;

/**
 * Contract that each regional Truco variant must implement.
 * Variants differ in card ranking, manilha rules, bid sequences, and winning score.
 */
public interface TrucoVariant {

    String getName();

    /** Points needed to win the match. */
    int getWinningScore();

    /**
     * Returns the strength of a card in this variant.
     * Higher value = stronger card. Used to determine trick winner.
     * @param card     the card being evaluated
     * @param revealed the "vira" card (null for variants with fixed manilhas)
     */
    int cardStrength(Card card, Card revealed);

    /** Ordered list of valid bid calls for the Truco sequence (e.g. Truco, Retruco, Vale Quatro). */
    List<BidCall> getTrucoBidSequence();

    /** Whether this variant includes Envido bidding. */
    boolean hasEnvido();
}
