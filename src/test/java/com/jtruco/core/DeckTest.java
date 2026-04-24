package com.jtruco.core;

import com.jtruco.core.card.Deck;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class DeckTest {

    @Test
    void deckHas40Cards() {
        Deck deck = new Deck();
        assertEquals(40, deck.remaining());
    }

    @Test
    void dealReducesCount() {
        Deck deck = new Deck();
        deck.deal();
        assertEquals(39, deck.remaining());
    }

    @Test
    void emptyDeckThrows() {
        Deck deck = new Deck();
        for (int i = 0; i < 40; i++) deck.deal();
        assertThrows(IllegalStateException.class, deck::deal);
    }
}
