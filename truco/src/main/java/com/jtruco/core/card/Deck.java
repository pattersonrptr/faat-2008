package com.jtruco.core.card;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class Deck {

    private final List<Card> cards = new ArrayList<>(40);

    public Deck() {
        for (Suit suit : Suit.values()) {
            for (Rank rank : Rank.values()) {
                cards.add(new Card(rank, suit));
            }
        }
    }

    public void shuffle() {
        Collections.shuffle(cards);
    }

    public Card deal() {
        if (cards.isEmpty()) throw new IllegalStateException("Deck is empty");
        return cards.removeLast();
    }

    public int remaining() {
        return cards.size();
    }
}
