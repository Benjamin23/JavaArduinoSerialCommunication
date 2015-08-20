package com.benjamin;

import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortDataListener;
import com.fazecast.jSerialComm.SerialPortEvent;

import javafx.fxml.FXML;
import javafx.scene.control.TextArea;
import javafx.scene.control.TextField;

public class MainController {

	@FXML
	private TextField tfCOM = new TextField();

	@FXML
	private TextField tfSSID = new TextField();

	@FXML
	private TextField tfPass = new TextField();

	@FXML
	private TextArea taIncoming = new TextArea();

	private SerialPort currentPort = null;

	/**
	 * Ova funkcija se poziva prilikom ucitavanja view-a kojem pripada ovaj kontroler.
	 * Pokrece funkciju za skeniranje portova.
	 */
	@FXML
	private void initialize() {
		taIncoming.setText("Initializing com ports...");
		taIncoming.appendText("\nSystem will use the first available port");
		handleScan();
	}

	/**
	 * Poziva se prilikom pritiska na tipku Send Data u view-u
	 * Dohvaca text iz polja za unos ssid-a i passworda te ih 
	 * salje ukoliko je serijski port otvoren.
	 */
	@FXML
	private void handleSend() {
		if (tfSSID.getText().length() > 0) {
			String response = "S:" + tfSSID.getText() + ";";
			if (tfPass.getText().length() > 0) {
				response += "P:" + tfPass.getText() + ";";
			}
			byte[] resData = response.getBytes();
			int bytesWritten = currentPort.writeBytes(resData, resData.length);
			System.out.println("Bytes written:" + bytesWritten);
		} else {
			checkIncomingTextArea();
			taIncoming.appendText("\nError, please enter a valid SSID");
		}
	}

	/**
	 * Poziva se prilikom pritiska na tipku Stop Connection.
	 * Poziva funkciju koja ce prekiniti odspojiti trenutno spojeni
	 * serijski port
	 */
	@FXML
	private void handleStop() {
		checkIncomingTextArea();
		closeCurrentPort();
	}

	/**
	 * Poziva se prilikom pritiska na Scan Ports
	 * Skenira sve dostupne serijske portove te ako ima 
	 * dostupnih, salje se funckiji za inicijaliziranje prvi
	 * dostupan.
	 */
	@FXML
	private void handleScan() {
		closeCurrentPort();
		SerialPort[] availablePorts = SerialPort.getCommPorts();
		String temp = "";
		for (SerialPort serialPort : availablePorts) {
			temp += serialPort.getSystemPortName() + " ";
		}
		tfCOM.setText(temp);
		checkIncomingTextArea();
		if (availablePorts.length > 0) {
			taIncoming.appendText("\nSystem will use port:" + availablePorts[0].getSystemPortName());
			initializePort(availablePorts[0]);
		} else {
			taIncoming.appendText("\nThere are no available ports, please scan again");
		}
	}

	/**
	 * Funkcija koja se spajan an predani port te inicijalizira listener na njemu
	 * koji ce reagirati na svaki primljeni podataka i dodati ga textu u text area polju
	 * 
	 * @param port Port na koji ce se pokusati spojiti
	 */
	private void initializePort(SerialPort port) {
		currentPort = port;
		port.openPort();
		port.addDataListener(new SerialPortDataListener() {
			@Override
			public int getListeningEvents() {
				return SerialPort.LISTENING_EVENT_DATA_AVAILABLE;
			}
			@Override
			public void serialEvent(SerialPortEvent event) {
				if (event.getEventType() != SerialPort.LISTENING_EVENT_DATA_AVAILABLE)
					return;
				byte[] newData = new byte[port.bytesAvailable()];
				int numRead = port.readBytes(newData, newData.length);
				System.out.println("Bytes read from serial port: " + numRead);
				checkIncomingTextArea();
				taIncoming.appendText(new String(newData));
			}
		});
	}

	/**
	 * Zatvaranje trenutno otvorenog porta
	 */
	private void closeCurrentPort() {
		if (currentPort != null && currentPort.isOpen()) {
			currentPort.closePort();
			taIncoming.appendText("\nPort: " + currentPort.getSystemPortName() + " is now closed");
		}
	}

	/**
	 * Ukoliko u text area polju ima vise od 10 redaka, 
	 * to polje se cisti. 
	 */
	private void checkIncomingTextArea() {
		String currentText = taIncoming.getText();
		int lineCounter = 0;
		for (char c : currentText.toCharArray()) {
			if (c == '\n') {
				lineCounter++;
			}
		}
		if (lineCounter >= 8) {
			taIncoming.setText("");
		}
	}
}
