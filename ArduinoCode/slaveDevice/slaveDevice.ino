/*
Autor: Benjamin Suk
Datum: 31.07.2015.
Koristeni hardware: 
  Arduino Pro Mini ATMEGA328P 5V
  NRF24L01+
  ACS712S
  Vanjski izvog napajanja (step down regulator) sa 230V na 5.5 900mA
  Regulator napona sa 5.5 na 3.3V

Napomena:
-nRF24L01: Dodan je keramicki kondenzator od 3.3uF izmedu VCC i GND nozica.
-Komentari: Serial.println funkcije su koristene za debugiranje tokom izrade sustava. U konacnoj
verziji sustava su zakomentiranje zbog ustede memorije na Arduino ploci. 
*/

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

/* 
CE_PIN oznacava pin na Arduinu gdje je spojen CE nrf24l01 modula. CE je Chip Enable, on aktivira tx ili rx mod na nrf-u.
CSN_PIN oznacava pin na Arduinu gdje je spojen CSN nrf24l01 modula. CSN je SPI Chip Select.
*/
#define CE_PIN 9
#define CSN_PIN 10

/*
ACS modul je spojen na analogni pin arduina
*/
#define ACS712_PIN 0

// RF24 Library koristi NRF module za komunikaciju
RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);

//Adrese za komunikaciju se krecu od 00 do 05
const uint16_t master_address = 00;
const uint16_t slave_address = 01;
// Kanal putem kojeg komuniciraju
const uint8_t channel = 90;

// Master i slave uredaji podatke razmjenjuju u ovom obliku
struct measuring {
	int node_id;
	float current_value;
} measured_value;

/*
 * Interval slanja podataka u milisekundama. te broj intervala.
 * Trenutno je postavljeno da svakih 30 sec uzima mjerenje i to tako
 * 12x. Nakon toga racuna srednju vrijednost za tih 12 mjerenje i salje 
 * vrijednost na master uredaj.
 */
const unsigned int numberOfIntervals = 12; 
const unsigned long interval = 30000;
unsigned long time_last;
int intervalCounter = 0;

/*
 * Interval za mjerenje struje u mikrosekundama.
 * Uzorkovanje se izvodi tijekom 100ms, uzima se
 * 250 uzoraka u intervalima od 400us.
 */
const unsigned long sampleTime = 100000UL;                           
const unsigned long numSamples = 250UL;                               
const unsigned long sampleInterval = sampleTime/numSamples;  
const int adc_zero = 510;
float sum = 0;


void setup()
{
  //Pokretanje svih kanala komunikacije
	Serial.begin(9600);
	SPI.begin();
	radio.begin();
	network.begin(channel, slave_address);

  //Postavljanje ID-a node-a radi lakseg raspoznavanja kasnije
	measured_value.node_id = 07;
	
	delay(1000);
}

void loop()
{
  
	network.update();
 
  //Mjerenje je odredeno varijablom interval
	if(millis()-time_last>interval) 
	{
		Serial.print("Measurement No: ");
		Serial.println(intervalCounter);
	 	time_last=millis();
   
    //Ukoliko je mjerenje izvrseno odredeni broj puta
		if(intervalCounter == numberOfIntervals)
		{
      //Izracunaj srednju vrijednost svih mjerenja i posalji tu vrijednost masteru
			Serial.print("Sending data to master device");
			Serial.println(sum/numberOfIntervals);
    	sendDataToMaster(sum/numberOfIntervals); 
    	sum = 0;
    	intervalCounter = 0;
	  } 
    //Ponovi mjerenje
    else 
    {
      //dohvati vrijednosti struje i pretvori u vate
    	float value = convertCurrentValueToWatts(getCurrentValue());
    	Serial.print("Value measured in Watts:");
    	Serial.println(value);    
  		sum += value;
  		intervalCounter++;
    }
	}
} 

/*
 * Mjerenje vrijednosti struje prema definiranim parametrima.
 * Uzorci se uzima odredeni broj puta odreden vrijednosti parametra numSamples.  
 * Uzorak se uzima svakih sampleInterval us.
 * Nakon sto se uzorak izmjerim, on se kvadrira te dodaje ukupnoj sumi. 
 * Nakon sto su uzerti svi uzorci, ukupna izmjerena vrijednost se dijeli sa brojm uzoraka, 
 * mnozi sa vrijednosti 75.7576/1024. Ta vrijednost je dobivena na sljedeci nacin. 
 * 75.7576 je omjer 5V/0.066, 0V-5V je raspon koji daje ACS, a 0.066mV je  
 * vrijednost koju ACS daje za ocitani 1A. 1024 je raspon koji ocitava analogni ulaz arduina. 
 * Konacna vrijednost se korjenuje te se dobiva efektivna vrijednost struje. 
 */
float getCurrentValue() {
	
	unsigned long currentAcc = 0;
	unsigned int count = 0;
	unsigned long prevMicros = micros() - sampleInterval ;
	while (count < numSamples)
	{
		if (micros() - prevMicros >= sampleInterval)
		{
			int adc_raw = analogRead(ACS712_PIN) - adc_zero;
      		currentAcc += (unsigned long)(adc_raw * adc_raw);
			++count;
			prevMicros += sampleInterval;
		}
	}
	
	float rms = sqrt((float)currentAcc/(float)numSamples) * (75.7576 / 1024.0);
  	return rms;
}
/*
 * Slanje podataka master cvoru preko definiranih adresa i kanala. 
 * Salje se id node-a koji je izmjerio iznos te izmjerena vrijednost. 
 */
void sendDataToMaster(float data) {

	measured_value.current_value = data;
  RF24NetworkHeader header = RF24NetworkHeader(master_address);
	boolean dataWriting = network.write(header, &measured_value, sizeof(measured_value));
	if(dataWriting) {
    Serial.println(data);
    Serial.println("Data send successfully");
	} else {
    Serial.println("Error");
	}

}

/*
Pretvaranje izmjerene vrijenosti struje u wate. U drugoj verziji bi bilo potrebno
dodati i mjerenje napona radi sto preciznijih rezultata. U trenutnoj verziji je
uzeta konstantna vrijednost napona od 230V.
*/
float convertCurrentValueToWatts(float current) {
	return current*230;
}
