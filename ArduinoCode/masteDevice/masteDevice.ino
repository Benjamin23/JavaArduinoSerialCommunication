/*
Autor: Benjamin Suk
Datum: 31.07.2015.
Koristeni hardware: 
  Arduino Nano ATMEGA328P 5V
  NRF24L01+
  ESP8266
  Logic Level Shifter 5V -> 3.3V
  Adjustable Breadboard Power Supply (5V & 3.3V exit)
  Vanjski izvog napajanja u obliku adaptera sa 230V na 12V 1A

Napomena:
-Heroku: Uredaj ne funkcionira sa Heroku servisom zbog toga sto Heroku
ne omogucava otvaranje TCP portova u nasoj regiji. Za US regiju postoji 
plugin koji to omogucava. 
-nRF24L01: Dodan je keramicki kondenzator od 3.3uF izmedu VCC i GND nozica.
-Komentari: Serial.println funkcije su koristene za debugiranje tokom izrade sustava. U konacnoj
verziji sustava su zakomentiranje zbog ustede memorije na Arduino ploci. 
*/
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

/* 
CE_PIN oznacava pin na Arduinu gdje je spojen CE nrf24l01 modula. CE je Chip Enable, on aktivira tx ili rx mod na nrf-u.
CSN_PIN oznacava pin na Arduinu gdje je spojen CSN nrf24l01 modula. CSN je SPI Chip Select.
*/
#define CE_PIN 9
#define CSN_PIN 10

// RF24 Library koristi NRF module za komunikaciju
RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);

// Adresa master uredaja i kanal putem kojeg ce master primati podatke
const uint16_t master_address = 00; 
const uint8_t channel = 90;

//EEPROM adresa od koje pocinje zapisivanje podataka o wifi mrezi
int eepromStartAddress = 0;

//Varijable koje odreduju trajanje config moda
long deviceStartUpTime;
const long interval = 10000;

//Globalne parsirane varijable koje sadrze podatke o wifi mrezi
String wifiSSID = "";
String wifiPass = "";

/*
  Programski serijski port, sluzi za komunikaciju sa ESP modulom. 
  ESP je spojen na pinove 5 (RX) i 6 (TX)
*/
SoftwareSerial mySerial(5, 6); // RX, TX

/*
Podatci o serveru na koji se spremaju vrijednosti
Za potrebe testiranja se uredaj spaja na ThingSpeak-ov server 
Potreban je otvoreni TCP port na serveru za uspjesno spajanje ESP 
modula. 
*/
//Thingspeak podatci
//String serverIP = "184.106.153.149";
//String port = "80";
//String apiKey = "GOFJT4U7IQT5GJN0";

//Web360 server podatci
String serverIP = "diplomski.web360.com.hr";
String port = "80";


// Master i slave uredaji podatke razmjenjuju u ovom obliku
struct measuring {
  int node_id;
  float current_value;
}; 

void setup()
{
  // Pocetna inicijalizacija i pokretanje svih portova i kanala
  Serial.begin(9600);
  mySerial.begin(9600);
  SPI.begin();
  radio.begin();
  // Dodavanje uredaja u kreiranu mrezu u odredeni kanal sa odredenom adresom
  network.begin(channel, master_address);

  delay(2000);


  /*
  Resetiranje ESP modula te ispisivanje svih dostupnih AP-ova u okolici.
  Ukoliko je uredaj spojen na aplikaciju za konfiguriranje, u textarea boxu
  ce biti ispisane sve dostupne mreze.
  */
  // Serial.println("The device will now prepare it's WiFi module. ");
  // Serial.println("It will restart the WiFi module and list all available networks");
  resetESPModule();
  listAvailableWifiNetworks();

  // Serial.println("Entering configuration mode");
  // Serial.print("Config period is ");
  // Serial.print(interval/1000);
  // Serial.println(" seconds");
  Serial.println("Config period 10s");
  delay(1000);
  deviceStartUpTime = millis();

  /*
   * Nakon isteka config perioda, ova funkcija ce vratiti ili prazan string 
   * ili podatke koje je dobila od aplikacije serijskom vezom
   */
  String dataRecived = enterConfigurationMode();
  // Serial.println("Device exited configuration mode");
  // Serial.println("Data recived over serial port:");
  // Serial.println(dataRecived);
  
  //Primljeni podatci se spremaju u EEPROM
  saveWifiConnectionDataToEEPROM(dataRecived);

  //Funkcija vraca true u slucaju da se uredaj uspjeno spojio na WiFi mrezu. 
  boolean connectedToWiFi = connectToWifi();

  //Isto koristeno za debugging ali sam ostavio
  if(connectedToWiFi) {
    Serial.println("REEEEADY TOOOO RUUUUUMBLEEEEEEE!!!!!!!!!!!!");
  } else {
    Serial.println("Not connected");
  }
}

void loop()
{

  //Update mreze prilikom svakog takta zbog osluskivanja za promjenama  
  network.update();
	
	while(network.available()) 
  {
		/*
    Ako je nesto dostupno na mrezi, parsira se i salje funkciji za upload.    
    */ 
    RF24NetworkHeader header;
		measuring recived_data;
		network.read(header, &recived_data, sizeof(recived_data));

    Serial.print("Recived from:");
	  Serial.println(recived_data.node_id);
	  Serial.print("On address:");
	  Serial.println(header.from_node);
	  Serial.print("Value recived:");
	  Serial.println(recived_data.current_value);
    uploadData(recived_data.current_value, recived_data.node_id);
  }
}

/*
 * Tokom trajanja config perioda, osluskuje se serijski port te se podatci dobiveni
 * spremaju u string. Nakon isteka se podatci vracaju na mjesto poziva funkcije
 */
String enterConfigurationMode() 
{
  String data = "";
  while(millis() - deviceStartUpTime < interval) \
  {
      if(Serial.available()) {
         while(Serial.available()) {
         char x = (char) Serial.read();
         data.concat(x);
       } 
    }
  }
  return data;
}

/*
 * Ukoliko su dobiveni podatci putem serijske veze, oni se spremaju u EEPROM, 
 * te se parsiraju. Ako nije nista primljeno, onda se citaju postojeci podatci iz EEPROM-a.
 */
void saveWifiConnectionDataToEEPROM(String data) 
{
  if(data.length() > 1) 
  {
        /*
        50 byte-ova EEPROM-a od pocetne adrese se resetira (postavlja na 0). 
        */
        for (int i=0;i<50;i++) 
        {
          EEPROM.write(i+eepromStartAddress, 0);
        }

        int dataLen = data.length()+1;
        char dataArray[dataLen];
        data.toCharArray(dataArray, dataLen);

        //Zapisivanje podataka u EEPROM 
        for (int i=0;i<dataLen;i++) 
        {
          EEPROM.write(eepromStartAddress+i, dataArray[i]);
        }

        parseWifiConnectionData(data);
    } 
    else 
    {
      // Serial.print("No data recived");
      readWifiConnectionDataFromEEPROM();
    }

}

/*
 * Citanje podataka iz EEPROM-a. Nakon citanja, podatci se parsiraju.
 */
void readWifiConnectionDataFromEEPROM() 
{
    String data = ""; 
    for (int i=0;i<50;i++) 
    {
      data.concat((char)EEPROM.read(i+eepromStartAddress));
    }
    data.trim();
    parseWifiConnectionData(data);
}

/*
 * Parsiranje podataka potrebnih za spajanje za wifi mrezu.
 * Podatci koji su spremljeni u EEPROM-u se nalaze u obliku:
 * S:<ssid>;P:<pass>; te ih je potrebno prebaciti u globalne 
 * varijable wifiSSID i wifiPass
 */
void parseWifiConnectionData(String data) 
{
  if(data.length()>1) 
  {
    int dataLen = data.length()+1;
    char dataArray[dataLen];
    data.toCharArray(dataArray, dataLen);
    boolean isSSID, isPass = false;

    for (int i=0;i<dataLen;i++) 
    {
        // Serial.print(dataArray[i]);
        // Ako se ne radi o predzadnjem znaku
        if(i!=dataLen-1) 
        {
            if(dataArray[i]=='S' && dataArray[i+1]==':') 
            {
                 isSSID = true;
            } 
            else if(dataArray[i]=='P' && dataArray[i+1]==':') 
            {
                 isPass = true;
            }  
        }
        
        if(isSSID==true && dataArray[i]!=';') 
        {
          wifiSSID.concat(dataArray[i]);
        } 
        else 
        {
          isSSID = false;
        }

        if(isPass==true && dataArray[i]!=';') 
        {
          wifiPass.concat(dataArray[i]);
        } 
        else 
        {
          isPass = false;
        }
    }
  } 

  wifiSSID = wifiSSID.substring(2);
  wifiPass = wifiPass.substring(2);

  Serial.println("Data after parsing SSID: ");
  Serial.println(wifiSSID);
  Serial.println("PASS:");
  Serial.println(wifiPass);
}

/*
Funkcija za spajanje na WiFi mrezu. 
*/
boolean connectToWifi() 
{
  // Serial.println("Changing ESP mode to station");
  sendCommandToESP("AT+CWMODE=1",3000);

  //Kreiranje naredbe za spajanje na wifi
  String wifiConnectionString = "AT+CWJAP=\"";
  wifiConnectionString.concat(wifiSSID);
  wifiConnectionString.concat("\",\"");
  wifiConnectionString.concat(wifiPass);
  wifiConnectionString.concat("\"");

  // Serial.println("Connecting to WiFi network");
  sendCommandToESP(wifiConnectionString,4000);
  
  // Serial.println("Assigned IP address");
  String response = sendCommandToESP("AT+CIFSR",1000);
  String ip = response.substring(0,response.length()-2); 
  ip.trim();
  // Serial.print("IP: ");  
  // Serial.println(ip);

  if(ip.substring(0,3)!= "192") 
  {
    // Serial.println("Error while connecting");
    // Serial.println("Please restart your device");
    return false;
  } 
  else 
  {
    return true;
  }
}

/*
Ispis svih dostupnih mreza. 
*/
void listAvailableWifiNetworks() 
{
  // Serial.println("Listing available wifi networks");
  String response = sendCommandToESP("AT+CWLAP",5000);
}

/*
Odspajanje ESP modula od pristupne tocke te njegovo resetiranje
*/
void resetESPModule() 
{
  // Serial.println("Disconnecting the device from AP"); 
  sendCommandToESP("AT+CWQAP",1000);
  // Serial.println("Reseting the device WiFi module");
  sendCommandToESP("AT+RST", 5000);
  // Serial.println("Device is now ready for connection to a network");
}


/*
Funkcija za upload podataka na server. 
Kao parametre prima vrijednost potrosene energije u Watima the id node-a koji
je ocitao tu vrijednost. 
*/
void uploadData(float value, int id) 
{
    // Serial.println("Enabling multiple connections");
    sendCommandToESP("AT+CIPMUX=1",1000);

    // Kreiranje nardbe za spajanje na server
    String serverConnection = "AT+CIPSTART=4,\"TCP\",\"";
    serverConnection.concat(serverIP);
    serverConnection.concat("\",");
    serverConnection.concat(port);

    // Serial.println("Connecting to the server");
    String res = sendCommandToESP(serverConnection,1000);
    
    if(res.substring(0,2)=="OK") 
    {
      // Serial.println("Successfully connected to server");
      // Kreiranje nardbe za slanje podataka na server
      String getRequest =  "GET /test.php?nodeid=";
      getRequest.concat(id);
      getRequest.concat("&value=");
      getRequest.concat(value);
      getRequest.concat(" HTTP/1.1\r\nHost: diplomski.web360.com.hr\r\n\r\n");

      // Duljina naredbe +2 zbog CR i NL sto se dodaju u sendCommandToESP funkciji 
      int reqLen = getRequest.length()+2;
      
      // Kanal je 4, definiran gore u nardbi za spajanje na server
      String dataSize = "AT+CIPSEND=4,";
      dataSize.concat(reqLen);

       // Serial.println("Sending the number of bytes to the server");  
       sendCommandToESP(dataSize,1000);
       // Serial.println("Sending the GET request");
       sendCommandToESP(getRequest,3000);
       // Serial.println("Closing the server connection");
       //Ako se stavi id da je 5, onda zatvara sve
       sendCommandToESP("AT+CIPCLOSE=4",1000);
    }
}

/*
Naredba za ESP se predaje u cistom obliku, a ovdje joj se dodaju CR i NL.
ESP-ov odgovor se parsira i vraca u returnu u cistom obliku. 
*/
String sendCommandToESP(String command, int delayTime) 
{
  Serial.print("Sending AT Command to ESP: ");
  Serial.println(command);
  int commandLength = command.length();

  // Naredbe za ESP moraju imati CR i NL ukljucen u sebi
  command.concat("\r\n");
  mySerial.print(command);
  // Delay od delayTime sekundi, neke naredbe ESP-a trebaju malo duze da se izvrse
  delay(delayTime);
  String response = "";
  while(mySerial.available()) 
  {
     response.concat((char) mySerial.read());
  }
  String finalResponse = response.substring(commandLength);
  finalResponse.trim();
  Serial.println("Response gotten from ESP:");
  Serial.println(finalResponse);

  return finalResponse;
}
