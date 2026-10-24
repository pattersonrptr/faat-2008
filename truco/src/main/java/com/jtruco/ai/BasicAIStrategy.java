package com.jtruco.ai;

import com.jtruco.core.card.Card;
import com.jtruco.core.game.BidCall;
import com.jtruco.core.game.BidResponse;
import com.jtruco.core.game.GameContext;
import com.jtruco.core.game.TrucoVariant;

import java.util.Comparator;
import java.util.List;
import java.util.Optional;

/**
 * Rule-based AI. Knows the game well but makes no deep look-ahead.
 *
 * Card-play rules (in priority order):
 *   1. Partner is already winning the trick → discard the weakest card.
 *   2. Can win the trick → play the weakest card that still wins.
 *   3. Cannot win the trick → discard the weakest card.
 *
 * Truco-call thresholds (hand strength sum):
 *   Call  / accept raise  : > 25
 *   Accept opponent call   : > 18
 *   Raise (Retruco)        : > 32
 *   Run (reject + give up) : ≤ 12 (only if score allows)
 */
public class BasicAIStrategy implements AIStrategy {

    private static final int THRESHOLD_CALL_TRUCO   = 25;
    private static final int THRESHOLD_ACCEPT_BID   = 18;
    private static final int THRESHOLD_RAISE_BID    = 32;
    private static final int THRESHOLD_RUN           = 12;

    @Override
    public Card chooseCard(GameContext ctx) {
        List<Card> hand = ctx.getMyHand();
        TrucoVariant variant = ctx.getVariant();
        Card vira = ctx.getRound().getVira();

        Comparator<Card> byStrength = Comparator.comparingInt(c -> variant.cardStrength(c, vira));

        // Rule 1: partner winning → shed weakest card
        if (ctx.isPartnerWinningCurrentTrick()) {
            return hand.stream().min(byStrength).orElseThrow();
        }

        Optional<Card> strongest = ctx.strongestCardInCurrentTrick();

        // Rule 2: can we win? → play minimum winning card
        if (strongest.isPresent()) {
            int beatThreshold = variant.cardStrength(strongest.get(), vira);
            Optional<Card> minWinner = hand.stream()
                    .filter(c -> variant.cardStrength(c, vira) > beatThreshold)
                    .min(byStrength);
            if (minWinner.isPresent()) return minWinner.get();
        }

        // Rule 3: leading or cannot win → discard weakest
        return hand.stream().min(byStrength).orElseThrow();
    }

    @Override
    public BidResponse respondToBid(GameContext ctx, BidCall call) {
        int strength = ctx.myHandStrength();
        int nextPoints = call.pointsIfAccepted();

        // Desperate situation: nearly losing — accept anything to keep playing
        if (ctx.opponentScore() >= ctx.winningScore() - 1) return BidResponse.ACCEPT;

        if (strength > THRESHOLD_RAISE_BID && canRaiseFrom(ctx, call)) return BidResponse.RAISE;
        if (strength > THRESHOLD_ACCEPT_BID) return BidResponse.ACCEPT;

        // Running costs 1 point (pointsIfRejected)
        return strength <= THRESHOLD_RUN ? BidResponse.REJECT : BidResponse.ACCEPT;
    }

    @Override
    public boolean shouldCallTruco(GameContext ctx) {
        // Don't call if already at max bid
        List<BidCall> sequence = ctx.getVariant().getTrucoBidSequence();
        int currentPoints = ctx.getRound().getCurrentBidPoints();
        boolean alreadyAtMax = sequence.stream()
                .noneMatch(b -> b.pointsIfAccepted() > currentPoints);
        if (alreadyAtMax) return false;

        return ctx.myHandStrength() > THRESHOLD_CALL_TRUCO;
    }

    private boolean canRaiseFrom(GameContext ctx, BidCall currentCall) {
        return ctx.getVariant().getTrucoBidSequence().stream()
                .anyMatch(b -> b.pointsIfAccepted() > currentCall.pointsIfAccepted());
    }
}
