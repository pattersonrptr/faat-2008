package com.jtruco.core.card;

public enum Suit {
    PAUS("Paus", "♣"),
    OUROS("Ouros", "♦"),
    COPAS("Copas", "♥"),
    ESPADAS("Espadas", "♠");

    private final String name;
    private final String symbol;

    Suit(String name, String symbol) {
        this.name = name;
        this.symbol = symbol;
    }

    public String getName() { return name; }
    public String getSymbol() { return symbol; }

    @Override
    public String toString() { return name; }
}
