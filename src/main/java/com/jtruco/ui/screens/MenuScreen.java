package com.jtruco.ui.screens;

import com.jtruco.core.game.GameController;
import com.jtruco.core.game.GameFactory;
import com.jtruco.core.game.TrucoVariant;
import com.jtruco.variants.paulista.PaulistaVariant;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.*;

import java.util.List;
import java.util.function.Consumer;

public class MenuScreen extends VBox {

    private final Consumer<GameController> onPlay;

    private TrucoVariant selectedVariant = new PaulistaVariant();
    private VBox selectedCard = null;

    public MenuScreen(Consumer<GameController> onPlay) {
        this.onPlay = onPlay;
        setId("menuRoot");
        setAlignment(Pos.CENTER);
        setSpacing(28);
        setPadding(new Insets(40));

        Label title    = new Label("JTruco");
        title.getStyleClass().add("menu-title");

        Label subtitle = new Label("Escolha o modo e jogue!");
        subtitle.getStyleClass().add("menu-subtitle");

        // Player name
        Label nameLabel = new Label("Seu nome:");
        nameLabel.getStyleClass().add("menu-field-label");
        TextField nameField = new TextField("Jogador");
        nameField.getStyleClass().add("menu-text-field");

        VBox nameBox = new VBox(6, nameLabel, nameField);
        nameBox.setAlignment(Pos.CENTER);

        // Variant selection
        Label variantLabel = new Label("Escolha o modo:");
        variantLabel.getStyleClass().add("menu-field-label");

        List<TrucoVariant> variants = List.of(new PaulistaVariant());
        HBox variantRow = buildVariantRow(variants);

        VBox variantBox = new VBox(10, variantLabel, variantRow);
        variantBox.setAlignment(Pos.CENTER);

        // Play button
        Button btnPlay = new Button("Jogar!");
        btnPlay.getStyleClass().add("btn-play");
        btnPlay.setOnAction(e -> {
            String name = nameField.getText().isBlank() ? "Jogador" : nameField.getText().trim();
            GameController controller = GameFactory.createSinglePlayerGame(name, selectedVariant);
            onPlay.accept(controller);
        });

        getChildren().addAll(title, subtitle, nameBox, variantBox, btnPlay);

        // Pre-select Paulista
        if (!variantRow.getChildren().isEmpty()) {
            selectVariantCard((VBox) variantRow.getChildren().getFirst(), variants.getFirst());
        }
    }

    private HBox buildVariantRow(List<TrucoVariant> variants) {
        HBox row = new HBox(16);
        row.setAlignment(Pos.CENTER);

        for (TrucoVariant v : variants) {
            VBox card = variantCard(v);
            card.setOnMouseClicked(e -> selectVariantCard(card, v));
            row.getChildren().add(card);
        }
        return row;
    }

    private VBox variantCard(TrucoVariant variant) {
        Label name = new Label(variant.getName());
        name.getStyleClass().add("variant-name");

        Label desc = new Label(variantDescription(variant));
        desc.getStyleClass().add("variant-desc");
        desc.setMaxWidth(200);

        VBox card = new VBox(8, name, desc);
        card.getStyleClass().add("variant-card");
        card.setMaxWidth(220);
        return card;
    }

    private void selectVariantCard(VBox card, TrucoVariant variant) {
        if (selectedCard != null) {
            selectedCard.getStyleClass().remove("variant-card-selected");
        }
        selectedCard = card;
        selectedVariant = variant;
        card.getStyleClass().add("variant-card-selected");
    }

    private String variantDescription(TrucoVariant v) {
        return switch (v.getName()) {
            case "Truco Paulista" -> "São Paulo. Manilhas fixas: Zap, Copas, Espadilha, Ouros. 12 pontos para vencer.";
            case "Truco Mineiro"  -> "Minas Gerais. Vira determina as manilhas a cada mão.";
            case "Truco Argentino"-> "Argentina. Com Envido. Baralho espanhol.";
            default               -> "";
        };
    }
}
