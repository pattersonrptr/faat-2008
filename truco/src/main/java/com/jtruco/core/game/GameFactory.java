package com.jtruco.core.game;

import com.jtruco.ai.BasicAIStrategy;
import com.jtruco.core.player.AIPlayer;
import com.jtruco.core.player.HumanPlayer;
import com.jtruco.core.player.Player;
import com.jtruco.core.team.Team;
import com.jtruco.variants.paulista.PaulistaVariant;

import java.util.List;

public final class GameFactory {

    private GameFactory() {}

    /**
     * Creates a standard 4-player 2v2 match.
     * Seating: [Human, OpponentA, Partner, OpponentB] — teams alternate seats.
     *
     * @param humanName the display name for the human player
     * @param variant   the Truco ruleset to use
     */
    public static GameController createSinglePlayerGame(String humanName, TrucoVariant variant) {
        HumanPlayer human   = new HumanPlayer(humanName);
        AIPlayer opponentA  = new AIPlayer("Adversário 1", new BasicAIStrategy());
        AIPlayer partner    = new AIPlayer("Parceiro",     new BasicAIStrategy());
        AIPlayer opponentB  = new AIPlayer("Adversário 2", new BasicAIStrategy());

        // Seating order determines play order: human → opponentA → partner → opponentB
        List<Player> players = List.of(human, opponentA, partner, opponentB);

        Team myTeam       = new Team("Nosso Time", human, partner);
        Team opponentTeam = new Team("Adversários", opponentA, opponentB);
        List<Team> teams  = List.of(myTeam, opponentTeam);

        Match match = new Match(variant, players, teams);
        return new GameController(match);
    }

    /** Convenience overload using Truco Paulista as default variant. */
    public static GameController createSinglePlayerGame(String humanName) {
        return createSinglePlayerGame(humanName, new PaulistaVariant());
    }
}
