package com.benjamin;

import javafx.fxml.FXML;
import javafx.scene.control.Alert;
import javafx.scene.control.Alert.AlertType;

/**
 * Ova klasa je povezana sa menu bar-om u RootLayout-u. 
 * Funkcije se odnose na menu bar iteme. 
 * Funckije su prilicno jednostavne, pa nema potrebe dodatno ih objasnjavati. 
 *
 */
public class MenuBarController {

	@FXML
	private void handleClose() {
		System.exit(0);
	}

	@FXML
	private void handleAbout() {
		Alert alert = new Alert(AlertType.INFORMATION);
		alert.setTitle("About");
		alert.setHeaderText("Application for configuration of the data collecting device");
		alert.setContentText("Benjamin Suk D629R\nThesis: Power consumption monitoring system");
		alert.showAndWait();
	}
}
