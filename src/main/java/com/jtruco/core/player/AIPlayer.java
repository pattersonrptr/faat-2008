package com.jtruco.core.player;

import com.jtruco.ai.AIStrategy;
import com.jtruco.core.card.Card;
import com.jtruco.core.game.BidCall;
import com.jtruco.core.game.BidResponse;
import com.jtruco.core.game.GameContext;

public class AIPlayer extends Player {

    private final AIStrategy strategy;

    public AIPlayer(String name, AIStrategy strategy) {
        super(name);
        this.strategy = strategy;
    }

    public Card decideCard(GameContext context) {
        return strategy.chooseCard(context);
    }

    public BidResponse decideBidResponse(GameContext context, BidCall call) {
        return strategy.respondToBid(context, call);
    }

    public boolean decideShouldCallTruco(GameContext context) {
        return strategy.shouldCallTruco(context);
    }

    @Override
    public boolean isHuman() { return false; }
}
