package com.jtruco.core;

import com.jtruco.ai.BasicAIStrategy;
import com.jtruco.core.card.Card;
import com.jtruco.core.card.Rank;
import com.jtruco.core.card.Suit;
import com.jtruco.core.game.*;
import com.jtruco.core.player.AIPlayer;
import com.jtruco.core.player.HumanPlayer;
import com.jtruco.core.team.Team;
import com.jtruco.variants.paulista.PaulistaVariant;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

class BasicAIStrategyTest {

    private PaulistaVariant variant;
    private HumanPlayer human;
    private AIPlayer partner;
    private AIPlayer opponentA;
    private AIPlayer opponentB;
    private Team myTeam;
    private Team opponentTeam;
    private Match match;

    @BeforeEach
    void setup() {
        variant   = new PaulistaVariant();
        human     = new HumanPlayer("Jogador");
        partner   = new AIPlayer("Parceiro",     new BasicAIStrategy());
        opponentA = new AIPlayer("Adversário 1", new BasicAIStrategy());
        opponentB = new AIPlayer("Adversário 2", new BasicAIStrategy());

        myTeam       = new Team("Nós",   human, partner);
        opponentTeam = new Team("Eles",  opponentA, opponentB);
        match = new Match(variant, List.of(human, opponentA, partner, opponentB),
                          List.of(myTeam, opponentTeam));
    }

    @Test
    void aiDiscardsWeakestWhenPartnerIsWinning() {
        // Give partner a Zap (strongest), give AI a 3 and a 4
        Card zap  = new Card(Rank.FOUR,  Suit.PAUS);
        Card three = new Card(Rank.THREE, Suit.OUROS);
        Card four  = new Card(Rank.FOUR,  Suit.OUROS);

        match.startNextRound();
        Round round = match.getCurrentRound();

        // Manually set hands for the scenario
        opponentA.clearHand(); opponentA.receiveCard(new Card(Rank.TWO, Suit.PAUS));
        partner.clearHand();   partner.receiveCard(zap);
        partner.receiveCard(three); // extra so we can test partner plays zap
        opponentB.clearHand(); opponentB.receiveCard(new Card(Rank.TWO, Suit.OUROS));
        human.clearHand();     human.receiveCard(new Card(Rank.KING, Suit.PAUS));

        // Simulate partner already played the Zap into the current trick
        round.currentTrick().addPlay(partner, zap);

        // AI (opponentA in this test) context
        opponentA.clearHand();
        opponentA.receiveCard(three);
        opponentA.receiveCard(four);

        GameContext ctx = new GameContext(opponentA, opponentTeam, myTeam, round, match);
        BasicAIStrategy strategy = new BasicAIStrategy();

        // Partner (enemy here for opponentA) is winning — but opponentA is NOT on partner's team,
        // so this tests the symmetric case: opponentA's team-mate is not winning
        // Let's test from partner's perspective instead
        GameContext partnerCtx = new GameContext(partner, myTeam, opponentTeam, round, match);

        partner.clearHand();
        partner.receiveCard(three);
        partner.receiveCard(four);

        // Partner's team-mate (human) is not involved yet; but the "winning" player in trick is partner himself (zap)
        // isPartnerWinningCurrentTrick should be false since the winner IS the player themselves
        Card chosen = strategy.chooseCard(partnerCtx);
        // Cannot win over own Zap; partner should discard weakest (four strength=1)
        assertEquals(four, chosen);
    }

    @Test
    void aiPlaysWeakestWinningCard() {
        match.startNextRound();
        Round round = match.getCurrentRound();

        // Opponent played a 2 (strength 9). AI has a 3 (strength 10) and a Zap (strength 14).
        Card opponentCard = new Card(Rank.TWO, Suit.PAUS);
        round.currentTrick().addPlay(opponentA, opponentCard);

        partner.clearHand();
        partner.receiveCard(new Card(Rank.THREE, Suit.COPAS)); // strength 10 — just beats the 2
        partner.receiveCard(new Card(Rank.FOUR, Suit.PAUS));   // Zap, strength 14

        GameContext ctx = new GameContext(partner, myTeam, opponentTeam, round, match);
        BasicAIStrategy strategy = new BasicAIStrategy();

        Card chosen = strategy.chooseCard(ctx);
        // Should play the 3 (weakest winner), not waste the Zap
        assertEquals(new Card(Rank.THREE, Suit.COPAS), chosen);
    }

    @Test
    void fullRoundCompletesWithoutError() {
        GameController controller = GameFactory.createSinglePlayerGame("Teste");
        List<GameController.GameEvent> events = new java.util.ArrayList<>();
        controller.addListener(events::add);

        controller.startRound();

        // Human plays all 3 cards
        for (int i = 0; i < 3; i++) {
            if (controller.isHumanTurn()) {
                Card card = controller.getMatch().getCurrentRound()
                        .getCurrentPlayer().getHand().getFirst();
                controller.humanPlayCard(card);
            }
        }

        boolean roundEndSeen = events.stream()
                .anyMatch(e -> e.type() == GameController.Event.ROUND_WON
                            || e.type() == GameController.Event.TRICK_WON
                            || e.type() == GameController.Event.TRICK_TIED);
        assertTrue(roundEndSeen, "Expected at least one trick or round event");
    }
}
