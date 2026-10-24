package com.jtruco.core.player;

import com.jtruco.core.card.Card;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public abstract class Player {

    private final String name;
    private final List<Card> hand = new ArrayList<>(3);

    protected Player(String name) {
        this.name = name;
    }

    public String getName() { return name; }

    public void receiveCard(Card card) {
        hand.add(card);
    }

    public List<Card> getHand() {
        return Collections.unmodifiableList(hand);
    }

    public void clearHand() {
        hand.clear();
    }

    public boolean hasCard(Card card) {
        return hand.contains(card);
    }

    public void removeCard(Card card) {
        hand.remove(card);
    }

    public abstract boolean isHuman();
}
