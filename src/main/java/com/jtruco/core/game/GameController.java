package com.jtruco.core.game;

import com.jtruco.core.card.Card;
import com.jtruco.core.player.AIPlayer;
import com.jtruco.core.player.Player;
import com.jtruco.core.team.Team;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.function.Consumer;

public class GameController {

    public enum Event {
        ROUND_STARTED,
        CARD_PLAYED,
        TRICK_WON,
        TRICK_TIED,
        ROUND_WON,
        TRUCO_CALLED,
        BID_ACCEPTED,
        BID_REJECTED,
        BID_DECISION_REQUIRED,   // emitted when human must respond to a bid
        MATCH_WON
    }

    public record GameEvent(Event type, String message, Object data) {}

    private final Match match;
    private final List<Consumer<GameEvent>> listeners = new ArrayList<>();

    // Bid-pause state — set when AI calls Truco and human must respond
    private boolean waitingForHumanBidResponse = false;
    private BidCall  pendingCall   = null;
    private AIPlayer pendingCaller = null;

    public GameController(Match match) {
        this.match = match;
    }

    public void addListener(Consumer<GameEvent> listener) {
        listeners.add(listener);
    }

    private void emit(Event type, String message) { emit(type, message, null); }

    private void emit(Event type, String message, Object data) {
        GameEvent event = new GameEvent(type, message, data);
        listeners.forEach(l -> l.accept(event));
    }

    public void startRound() {
        waitingForHumanBidResponse = false;
        pendingCall   = null;
        pendingCaller = null;
        match.startNextRound();
        emit(Event.ROUND_STARTED, "Nova mão começou.");
        advanceIfAI();
    }

    /** Called by the UI when the human plays a card. */
    public void humanPlayCard(Card card) {
        if (waitingForHumanBidResponse) throw new IllegalStateException("Aguardando resposta do truco.");
        Round round = match.getCurrentRound();
        Player current = round.getCurrentPlayer();
        if (!current.isHuman())          throw new IllegalStateException("Não é sua vez.");
        if (!current.hasCard(card))      throw new IllegalArgumentException("Você não tem essa carta.");
        executePlay(current, card, round);
    }

    /** Called by the UI when the human responds to a Truco bid. */
    public void humanRespondToBid(BidResponse response) {
        if (!waitingForHumanBidResponse) throw new IllegalStateException("Nenhum truco pendente.");
        waitingForHumanBidResponse = false;
        Round round = match.getCurrentRound();
        applyBidResponse(response, pendingCaller, pendingCall, round,
                         match.getTeams().stream().filter(t -> t.contains(
                             round.getPlayers().stream().filter(Player::isHuman).findFirst().orElseThrow()
                         )).findFirst().orElseThrow());
        pendingCall   = null;
        pendingCaller = null;
        if (!round.isFinished()) advanceIfAI();
    }

    /** Called by UI when the human proactively calls Truco. */
    public void humanCallTruco() {
        if (waitingForHumanBidResponse) return;
        Round round = match.getCurrentRound();
        Player human = round.getPlayers().stream().filter(Player::isHuman).findFirst().orElseThrow();
        Team humanTeam = teamOf(human);

        List<BidCall> sequence = match.getVariant().getTrucoBidSequence();
        BidCall nextCall = sequence.stream()
                .filter(b -> b.pointsIfAccepted() > round.getCurrentBidPoints())
                .findFirst().orElse(null);
        if (nextCall == null) return;

        emit(Event.TRUCO_CALLED, human.getName() + " pediu " + nextCall.name() + "!");

        // Opponent team responds — their first AI player decides
        Team opponentTeam = match.getTeams().stream().filter(t -> !t.equals(humanTeam)).findFirst().orElseThrow();
        AIPlayer responder = (AIPlayer) opponentTeam.getPlayers().getFirst();
        BidResponse response = responder.decideBidResponse(buildContext(responder, round), nextCall);
        applyBidResponse(response, (AIPlayer) human, nextCall, round, opponentTeam);
    }

    // ── private helpers ──────────────────────────────────────────────────────

    private void advanceIfAI() {
        Round round = match.getCurrentRound();
        while (!round.isFinished() && !waitingForHumanBidResponse && !round.getCurrentPlayer().isHuman()) {
            Player current = round.getCurrentPlayer();
            AIPlayer ai = (AIPlayer) current;

            GameContext ctx = buildContext(ai, round);
            if (ai.decideShouldCallTruco(ctx)) {
                handleTrucoCall(ai, round);
                if (round.isFinished() || waitingForHumanBidResponse) break;
            }

            if (!waitingForHumanBidResponse) {
                Card chosen = ai.decideCard(buildContext(ai, round));
                executePlay(ai, chosen, round);
            }
        }
    }

    private void executePlay(Player player, Card card, Round round) {
        round.playCard(player, card);
        emit(Event.CARD_PLAYED, player.getName() + " jogou " + card + ".", card);

        Trick lastCompleted = findLastCompletedTrick(round);
        if (lastCompleted != null) {
            Optional<Player> winner = lastCompleted.getWinner();
            if (winner.isPresent()) {
                emit(Event.TRICK_WON, winner.get().getName() + " ganhou a rodadinha.", winner.get());
            } else {
                emit(Event.TRICK_TIED, "Empate na rodadinha.");
            }
        }

        if (round.isFinished()) {
            finishRound(round);
            return;
        }
        advanceIfAI();
    }

    private void handleTrucoCall(AIPlayer caller, Round round) {
        List<BidCall> sequence = match.getVariant().getTrucoBidSequence();
        BidCall nextCall = sequence.stream()
                .filter(b -> b.pointsIfAccepted() > round.getCurrentBidPoints())
                .findFirst().orElse(null);
        if (nextCall == null) return;

        emit(Event.TRUCO_CALLED, caller.getName() + " pediu " + nextCall.name() + "!");

        Team callerTeam  = teamOf(caller);
        Team respondTeam = match.getTeams().stream().filter(t -> !t.equals(callerTeam)).findFirst().orElseThrow();
        Player responder = respondTeam.getPlayers().getFirst();

        if (responder.isHuman()) {
            waitingForHumanBidResponse = true;
            pendingCall   = nextCall;
            pendingCaller = caller;
            emit(Event.BID_DECISION_REQUIRED, "Você quer aceitar o " + nextCall.name() + "?", nextCall);
        } else {
            AIPlayer aiResponder = (AIPlayer) responder;
            BidResponse response = aiResponder.decideBidResponse(buildContext(aiResponder, round), nextCall);
            applyBidResponse(response, caller, nextCall, round, respondTeam);
        }
    }

    private void applyBidResponse(BidResponse response, AIPlayer caller, BidCall call,
                                   Round round, Team respondingTeam) {
        Player responder = respondingTeam.getPlayers().getFirst();
        switch (response) {
            case ACCEPT -> {
                round.setCurrentBidPoints(call.pointsIfAccepted());
                emit(Event.BID_ACCEPTED, responder.getName() + " aceitou. Vale " + call.pointsIfAccepted() + " ponto(s).");
            }
            case RAISE -> {
                round.setCurrentBidPoints(call.pointsIfAccepted());
                emit(Event.BID_ACCEPTED, responder.getName() + " aumentou!");
            }
            case REJECT -> {
                int earned = call.pointsIfRejected();
                teamOf(caller).addPoints(earned);
                emit(Event.BID_REJECTED, responder.getName() + " correu. +" + earned + " ponto(s) para " + teamOf(caller).getName() + ".");
                // Force the round to end by marking it finished via score threshold trick:
                // We trigger finishRound directly since a run ends the hand.
                finishRound(round);
            }
        }
    }

    private void finishRound(Round round) {
        match.applyRoundResult();
        Optional<Team> roundWinner = round.determineRoundWinner();
        roundWinner.ifPresent(t -> emit(Event.ROUND_WON, t.getName() + " venceu a mão!", t));

        if (match.isOver()) {
            match.getMatchWinner().ifPresent(t ->
                    emit(Event.MATCH_WON, t.getName() + " venceu o jogo!", t));
        }
    }

    private GameContext buildContext(Player player, Round round) {
        Team myTeam  = teamOf(player);
        Team oppTeam = match.getTeams().stream().filter(t -> !t.equals(myTeam)).findFirst().orElseThrow();
        return new GameContext(player, myTeam, oppTeam, round, match);
    }

    private Team teamOf(Player player) {
        return match.getTeams().stream()
                .filter(t -> t.contains(player))
                .findFirst().orElseThrow();
    }

    private Trick findLastCompletedTrick(Round round) {
        List<Trick> tricks = round.getTricks();
        for (int i = tricks.size() - 1; i >= 0; i--) {
            if (tricks.get(i).isComplete(match.getPlayers().size())) return tricks.get(i);
        }
        return null;
    }

    public Match  getMatch()                    { return match; }
    public boolean isHumanTurn()               { return !waitingForHumanBidResponse && match.getCurrentRound() != null && match.getCurrentRound().getCurrentPlayer().isHuman(); }
    public boolean isWaitingForHumanBidResponse() { return waitingForHumanBidResponse; }
    public BidCall getPendingCall()             { return pendingCall; }
}
