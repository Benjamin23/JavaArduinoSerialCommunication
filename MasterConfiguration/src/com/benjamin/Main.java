package com.benjamin;

import java.io.IOException;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Scene;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.BorderPane;
import javafx.stage.Stage;

public class Main extends Application {

	private Stage primaryStage;
	private BorderPane rootLayout;

	/**
	 * Start funkcija se poziva prilikom pokretanja aplikacije. 
	 * Postavlja primary stage i naslov te potrece funkcije za postavljanje 
	 * sadrzaja.
	 */
	@Override
	public void start(Stage primaryStage) {
		this.primaryStage = primaryStage;
		this.primaryStage.setTitle("Configuration Application");
		initRootLayout();
		showConfigurationView();
	}

	/**
	 * main funkcija kao i kod svake druge Java applikacije
	 * @param args
	 */
	public static void main(String[] args) {
		launch(args);
	}

	/**
	 * Inicijalizacija glavnog stage-a i postavljanje scene na njega
	 */
	public void initRootLayout() {
		try {
			FXMLLoader loader = new FXMLLoader();
			loader.setLocation(Main.class.getResource("view/RootLayout.fxml"));
			rootLayout = (BorderPane) loader.load();
			Scene scene = new Scene(rootLayout);
			primaryStage.setScene(scene);
			primaryStage.show();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	/**
	 * Ucitavanje FXML file-a sa main view-om te njegovo postavljanje 
	 * na scenu. 
	 */
	public void showConfigurationView() {
		try {
			FXMLLoader loader = new FXMLLoader();
			loader.setLocation(Main.class.getResource("view/MainView.fxml"));
			AnchorPane mainView = (AnchorPane) loader.load();
			rootLayout.setCenter(mainView);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
