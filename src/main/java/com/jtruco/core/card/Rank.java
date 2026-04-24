package com.jtruco.core.card;

public enum Rank {
    ACE("A", 1),
    TWO("2", 2),
    THREE("3", 3),
    FOUR("4", 4),
    FIVE("5", 5),
    SIX("6", 6),
    SEVEN("7", 7),
    JACK("J", 8),
    QUEEN("Q", 9),
    KING("K", 10);

    private final String label;
    private final int faceValue;

    Rank(String label, int faceValue) {
        this.label = label;
        this.faceValue = faceValue;
    }

    public String getLabel() { return label; }
    public int getFaceValue() { return faceValue; }

    @Override
    public String toString() { return label; }
}
