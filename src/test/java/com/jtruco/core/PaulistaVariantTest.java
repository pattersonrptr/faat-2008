package com.jtruco.core;

import com.jtruco.core.card.Card;
import com.jtruco.core.card.Rank;
import com.jtruco.core.card.Suit;
import com.jtruco.variants.paulista.PaulistaVariant;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class PaulistaVariantTest {

    private final PaulistaVariant variant = new PaulistaVariant();

    @Test
    void zapIsStrongest() {
        Card zap       = new Card(Rank.FOUR,  Suit.PAUS);
        Card copas     = new Card(Rank.SEVEN, Suit.COPAS);
        Card espadilha = new Card(Rank.ACE,   Suit.ESPADAS);
        Card ouros     = new Card(Rank.SEVEN, Suit.OUROS);
        Card three     = new Card(Rank.THREE, Suit.PAUS);

        int zapStr = variant.cardStrength(zap, null);
        assertTrue(zapStr > variant.cardStrength(copas, null));
        assertTrue(variant.cardStrength(copas, null) > variant.cardStrength(espadilha, null));
        assertTrue(variant.cardStrength(espadilha, null) > variant.cardStrength(ouros, null));
        assertTrue(variant.cardStrength(ouros, null) > variant.cardStrength(three, null));
    }

    @Test
    void threeBeatsTwo() {
        Card three = new Card(Rank.THREE, Suit.PAUS);
        Card two   = new Card(Rank.TWO,   Suit.PAUS);
        assertTrue(variant.cardStrength(three, null) > variant.cardStrength(two, null));
    }

    @Test
    void winningScoreIs12() {
        assertEquals(12, variant.getWinningScore());
    }
}
