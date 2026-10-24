package com.jtruco.ui;

import com.jtruco.ui.screens.GameTableScreen;
import com.jtruco.ui.screens.MenuScreen;
import javafx.application.Application;
import javafx.scene.Scene;
import javafx.stage.Stage;

public class JTrucoApp extends Application {

    private Stage primaryStage;

    @Override
    public void start(Stage stage) {
        this.primaryStage = stage;
        stage.setTitle("JTruco");
        stage.setMinWidth(900);
        stage.setMinHeight(680);
        showMenu();
        stage.show();
    }

    private void showMenu() {
        MenuScreen menu = new MenuScreen(controller -> {
            GameTableScreen table = new GameTableScreen(controller);
            Scene scene = new Scene(table, 1100, 780);
            scene.getStylesheets().add(
                    getClass().getResource("/com/jtruco/ui/style.css").toExternalForm());
            primaryStage.setScene(scene);
        });

        Scene menuScene = new Scene(menu, 900, 680);
        menuScene.getStylesheets().add(
                getClass().getResource("/com/jtruco/ui/style.css").toExternalForm());
        primaryStage.setScene(menuScene);
    }

    public static void main(String[] args) {
        launch(args);
    }
}
