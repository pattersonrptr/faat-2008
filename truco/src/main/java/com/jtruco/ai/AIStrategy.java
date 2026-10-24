package com.jtruco.ai;

import com.jtruco.core.card.Card;
import com.jtruco.core.game.BidCall;
import com.jtruco.core.game.BidResponse;
import com.jtruco.core.game.GameContext;

public interface AIStrategy {

    /** Choose a card to play from the AI's hand. */
    Card chooseCard(GameContext context);

    /** Decide how to respond when the opponent calls a Truco bid. */
    BidResponse respondToBid(GameContext context, BidCall call);

    /** Decide whether to proactively call Truco (or escalate). */
    boolean shouldCallTruco(GameContext context);
}
