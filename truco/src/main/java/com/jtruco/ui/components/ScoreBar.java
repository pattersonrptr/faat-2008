package com.jtruco.ui.components;

import com.jtruco.core.team.Team;
import javafx.geometry.Pos;
import javafx.scene.control.Label;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.Region;

import java.util.List;

public class ScoreBar extends HBox {

    private final Label myScore;
    private final Label oppScore;

    public ScoreBar(List<Team> teams, String variantName) {
        setId("scoreBar");
        setAlignment(Pos.CENTER);
        setSpacing(12);
        setMinHeight(48);

        Team myTeam  = teams.get(0);
        Team oppTeam = teams.get(1);

        Label myName  = new Label(myTeam.getName() + ":");
        myName.getStyleClass().add("score-label");

        myScore = new Label("0");
        myScore.getStyleClass().add("score-value");

        Region spacer1 = new Region();
        HBox.setHgrow(spacer1, Priority.ALWAYS);

        Label variant = new Label(variantName);
        variant.getStyleClass().add("variant-label");

        Region spacer2 = new Region();
        HBox.setHgrow(spacer2, Priority.ALWAYS);

        Label oppName = new Label(oppTeam.getName() + ":");
        oppName.getStyleClass().add("score-label");

        oppScore = new Label("0");
        oppScore.getStyleClass().add("score-value");

        getChildren().addAll(myName, myScore, spacer1, variant, spacer2, oppName, oppScore);
    }

    public void update(int my, int opp) {
        myScore.setText(String.valueOf(my));
        oppScore.setText(String.valueOf(opp));
    }
}
