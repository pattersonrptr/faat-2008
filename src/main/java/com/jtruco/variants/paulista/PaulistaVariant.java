package com.jtruco.variants.paulista;

import com.jtruco.core.card.Card;
import com.jtruco.core.card.Rank;
import com.jtruco.core.card.Suit;
import com.jtruco.core.game.BidCall;
import com.jtruco.core.game.TrucoVariant;

import java.util.List;

/**
 * Truco Paulista rules.
 *
 * Manilhas are fixed (no vira):
 *   1st: 4♣ (Zap)
 *   2nd: 7♥ (Copas)
 *   3rd: A♠ (Espadilha)
 *   4th: 7♦ (Ouros)
 *
 * Card strength (descending): Zap > Copas > Espadilha > Ouros >
 *   3 > 2 > A > K > J > Q > 7(♣,♥,♠) > 6 > 5 > 4
 *
 * First to 12 points wins.
 */
public class PaulistaVariant implements TrucoVariant {

    private static final List<BidCall> BID_SEQUENCE = List.of(
            new BidCall("Truco",      3, 1),
            new BidCall("Retruco",    6, 3),
            new BidCall("Vale Quatro", 12, 6)
    );

    @Override
    public String getName() { return "Truco Paulista"; }

    @Override
    public int getWinningScore() { return 12; }

    @Override
    public int cardStrength(Card card, Card vira) {
        // Manilhas (fixed, vira is ignored in Paulista)
        if (card.getRank() == Rank.FOUR  && card.getSuit() == Suit.PAUS)    return 14; // Zap
        if (card.getRank() == Rank.SEVEN && card.getSuit() == Suit.COPAS)   return 13; // Copas
        if (card.getRank() == Rank.ACE   && card.getSuit() == Suit.ESPADAS) return 12; // Espadilha
        if (card.getRank() == Rank.SEVEN && card.getSuit() == Suit.OUROS)   return 11; // Ouros

        return switch (card.getRank()) {
            case THREE -> 10;
            case TWO   -> 9;
            case ACE   -> 8;
            case KING  -> 7;
            case JACK  -> 6;
            case QUEEN -> 5;
            case SEVEN -> 4; // non-manilha sevens (♣ and ♠ sevens don't exist as manilhas)
            case SIX   -> 3;
            case FIVE  -> 2;
            case FOUR  -> 1; // non-manilha four
        };
    }

    @Override
    public List<BidCall> getTrucoBidSequence() { return BID_SEQUENCE; }

    @Override
    public boolean hasEnvido() { return false; }
}
