package com.jtruco.core.game;

public record BidCall(String name, int pointsIfAccepted, int pointsIfRejected) {

    @Override
    public String toString() { return name; }
}
