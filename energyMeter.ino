#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 7, 6, 5, 4, 3, 2);

// Variablen
float numberOfSamples = 250.0;	// Anzahl der Messungen über die gemittelt wird
float fSampleA = 0.0;	// Strom Wert
float fAmperage = 0.0;	// Stromstärke
float fAmperageOld = 0.0;	// Stromstärke der vorhergehenden Berechnung(en)
float fSampleV = 0.0;	// Spannungs Wert
float fVoltage = 0.0;	// Spannung
float fVoltageOld = 0.0;	// Spannung der vorhergehenden Berchnung(en)
float fSampleVBat = 0.0;  // Spannungs Wert Batterie
float fVoltageBat = 0.0; // Spannung Batterie
float fVoltageOldBat = 0.0;  // Spannung Batterie der vorhergehenden Berchnung(en)
long lTime = 0;	// aktuelle Zeit
long lTimeOld = 0;	// Zeit des vorhergehenden Berechnungszyklus
float fPower = 0.0;	// Leistung
float fEnergy = 0.0;	// Arbeit berechnen

void setup() 
{
  Serial.begin(9600);

  // initialize SD Card
  if (!SD.begin(10)) { Serial.println("card initialization failed"); return; }
  Serial.println("card initialization successfull");
  File logFile = SD.open("logFile.csv", FILE_WRITE);
  if(logFile) // write table header
  {
    logFile.println("Zeit [ms]; Stromstaerke [mA]; Spannung [mV]; Spannung Bat [mV]");
    Serial.println("Zeit [ms]; Stromstaerke [mA]; Spannung [mV]; Spannung Bat [mV]; Leistung [W]; Arbeit [Wh]; extra Variable [#]");
    logFile.close();
  }

  // initialize LCD display
  pinMode(9, OUTPUT); //pin 9 as output
  analogWrite(9, 35); // set output of pin 9 (backlight intensity 0-254)
  lcd.begin(16,2); // display size (columns, rows)
}

void loop()
{
  // Strom und Spannungs Eingangssignal ermitteln
  for(int i = 0; i < numberOfSamples; i++)
  {
    fSampleA = fSampleA + analogRead(A3); // Strom
    fSampleV = fSampleV + analogRead(A2); // Spannung
    fSampleVBat = fSampleVBat + analogRead(A4); // Spannung Batterie
    delay(5);
  }
  fSampleA = fSampleA / numberOfSamples;
  fSampleV = fSampleV / numberOfSamples;
  fSampleVBat = fSampleVBat / numberOfSamples;

  lTime = millis(); // Arduino Laufzeit in ms
  
  // Berechnungen
  fAmperage = (fSampleA - 513.5) / 20.54 * (-1.0);	// Stromstärke [A] berechnen
  fVoltage = 5.5 * 4.82 * fSampleV / 1000.0;	// Spannung [V] berechnen
  fVoltageBat = 5.5 * 4.82 * fSampleVBat / 1000.0;  // Spannung Batterie [V] berechnen
  fPower = fVoltage * fAmperage;	// Leistung [W] berechnen
  fEnergy += fPower * (lTime - lTimeOld) / 1000.0 / 3600.0;	// Arbeit [Wh] berechnen
  lTimeOld = lTime;	// alte Zeit speichern

  // Zeit, Strom und Spannung zu Ausgabestring hinzufuegen
  String dataString = "";
  dataString += String(lTime) + ";"; // Zeit [ms]
  dataString += String(fAmperage * 1000.0) + ";";  // Stromstaerke [mA]
  dataString += String(fVoltage * 1000.0) + ";";  // Spannung [mV]
  dataString += String(fVoltageBat * 1000.0);  // Spannung Batterie [mV]

  // Daten auf SD Karte speichern wenn Grenzwert im Vergleich zur vorhergehenden Messung ueber- / unterschritten wird
  if(fAmperage > fAmperageOld + 0.025 
	  || fAmperage < fAmperageOld - 0.025 
	  || fVoltage > fVoltageOld + 0.015 
	  || fVoltage < fVoltageOld - 0.015 
	  || fVoltageBat > fVoltageOldBat + 0.015 
	  || fVoltageBat < fVoltageOldBat - 0.015)
  {
	fAmperageOld = fAmperage;
	fVoltageOld = fVoltage;
	fVoltageOldBat = fVoltageBat;

	File logFile = SD.open("logFile.csv", FILE_WRITE);
	if(logFile)
	{
		logFile.println(dataString);
		logFile.close();
	}
	else { Serial.println("could not save data to file"); }
  }

  // Energie, Arbeit und extra Variable zu Ausgabestring ergaenzen
  dataString += ";" + String(fPower) + ";";  // Leistung [W]
  dataString += String(fEnergy) + ";"; // Arbeit [Wh]
  dataString += String(fSampleVBat);  // extra Variable [#]

  // serielle Ausgabe
  Serial.println(dataString);

  //LCD Ausgabe
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("B" + String(fVoltageBat) + "V P" + String(fPower) + "W");
  lcd.setCursor(0,1); lcd.print("Ges " + String(fEnergy) + "Wh");

  // naechste Messung um 5s verzögern
  delay(5000);
}
