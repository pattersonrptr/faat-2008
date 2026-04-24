package com.jtruco.ui.components;

import com.jtruco.core.game.BidResponse;
import com.jtruco.core.game.GameController;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.Region;
import javafx.scene.layout.VBox;

public class ActionBar extends VBox {

    public enum Mode { PLAY, BID_RESPONSE, ROUND_OVER, MATCH_OVER }

    private final Label  message;
    private final Button btnTruco;
    private final Button btnAccept;
    private final Button btnRaise;
    private final Button btnRun;
    private final Button btnNextRound;
    private final HBox   buttonRow;

    private Runnable onTruco;
    private Runnable onAccept;
    private Runnable onRaise;
    private Runnable onRun;
    private Runnable onNextRound;

    public ActionBar() {
        setId("actionBar");
        setSpacing(8);
        setAlignment(Pos.CENTER);

        message = new Label("Bem-vindo ao JTruco!");
        message.getStyleClass().add("message-label");

        btnTruco     = button("Truco!",   "btn-truco",     () -> fire(onTruco));
        btnAccept    = button("Aceitar",   "btn-accept",    () -> fire(onAccept));
        btnRaise     = button("Aumentar",  "btn-raise",     () -> fire(onRaise));
        btnRun       = button("Correr",    "btn-run",       () -> fire(onRun));
        btnNextRound = button("Próxima mão ▶", "btn-next-round", () -> fire(onNextRound));

        buttonRow = new HBox(10, btnTruco, btnAccept, btnRaise, btnRun, btnNextRound);
        buttonRow.setAlignment(Pos.CENTER);

        getChildren().addAll(message, buttonRow);
        setMode(Mode.PLAY, "Nova mão — clique em uma carta para jogar.", false);
    }

    public void setMode(Mode mode, String msg, boolean canRaise) {
        message.setText(msg);
        btnTruco.setVisible(false);
        btnAccept.setVisible(false);
        btnRaise.setVisible(false);
        btnRun.setVisible(false);
        btnNextRound.setVisible(false);

        switch (mode) {
            case PLAY -> btnTruco.setVisible(true);
            case BID_RESPONSE -> {
                btnAccept.setVisible(true);
                btnRaise.setVisible(canRaise);
                btnRun.setVisible(true);
            }
            case ROUND_OVER, MATCH_OVER -> btnNextRound.setVisible(true);
        }
    }

    public void setMessage(String msg) { message.setText(msg); }

    public void onTruco(Runnable r)     { onTruco     = r; }
    public void onAccept(Runnable r)    { onAccept    = r; }
    public void onRaise(Runnable r)     { onRaise     = r; }
    public void onRun(Runnable r)       { onRun       = r; }
    public void onNextRound(Runnable r) { onNextRound = r; }

    private static Button button(String text, String styleClass, Runnable action) {
        Button btn = new Button(text);
        btn.getStyleClass().add(styleClass);
        btn.setOnAction(e -> action.run());
        btn.setVisible(false);
        return btn;
    }

    private static void fire(Runnable r) { if (r != null) r.run(); }
}
