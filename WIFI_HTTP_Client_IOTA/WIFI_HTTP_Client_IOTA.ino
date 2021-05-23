//Purpose:
//  EMD With WIFI
//
//Hardware
//  MFNode
//  ESP8266

//Author
//  Kelvin Mwega (kelvin.mwega@iota.org)


#include "SoftwareSerial.h"
#include <Wire.h>
#include <avr/wdt.h>
#include "DHT.h"

#define LED13 13 
#define GSM_IGN 6  
#define reedPin 11
#define powerPin 12
#define lightPin A0
#define DHTPIN 4
#define DHTTYPE DHT11   

DHT dht(DHTPIN, DHTTYPE);

String ssid ="Vunyx R";
String password="Zazone2345";

SoftwareSerial esp(3, 2);// RX, TX

String data = "";
String server = "192.168.1.108";
String uri = "/clients/wifi";

long failures = 0;
int defaultLightOn = 500;
String quality;
long previousMillis = 0;

unsigned long startT = 999000;     // just bigger than interval :-)
unsigned long interval = 300000;   // 5 minutes !

unsigned long ledstartT = 999000;     // just bigger than interval :-)
unsigned long ledinterval = 20000;   // 5 minutes !

unsigned long batstartT = 999000;     // just bigger than interval :-)
unsigned long batinterval = 240000;   // 5 minutes !

String imei = "EMDWIFI01";

String batt = "0";

// Variables will change:
int doorStateCounter = 0, powerStateCounter = 0;  
int doorState = 0, powerState = 0;         
int lastDoorState = 0, lastPowerState = 0;    

byte dat [5];
int i = 0;

void setup() {
  esp.begin(9600);
  Serial.begin(9600);
  delay(2000);
  reset();
  connectWifi(); 

  digitalWrite(lightPin, INPUT_PULLUP);
  pinMode(reedPin, INPUT);
  pinMode(powerPin, INPUT);
  
  dht.begin();
  delay(1000);
 
  blinkLED(13,2,500);
}

void reset() {
  esp.println("AT+RST");
  delay(1000);
  if(esp.find("OK") ) Serial.println("Module Reset");
  delay(1000); 
} 


void connectWifi() {

  String cmd = "AT+CWJAP=\"" +ssid+"\",\"" + password + "\"";
  esp.println(cmd);
  delay(4000);
  if(esp.find("OK")) {
    Serial.println("Connected!");
  } 
else {
  connectWifi();
  Serial.println("Cannot connect to wifi"); 
  }
}

void loop() {
  wdt_reset();
  unsigned long loopT = millis();
  unsigned long battLoopT = millis();
  
  if ((unsigned long)(loopT - startT) >= interval){
    
    delay(100);
    data = getSensorVals(); 
    i = i++;
    httppost();
    startT = loopT;
  }
  
  delay(2000);
}

void httppost () {
  
  esp.println("AT+CIPSTART=\"TCP\",\"" + server + "\",1880");//start a TCP connection.
  
  if( esp.find("OK")) {
    Serial.println("TCP connection ready");
  } 
  
  delay(1000);
  
  String postRequest =
  "POST " + uri + " HTTP/1.0\r\n" +
  "Host: " + server + "\r\n" +
  "Accept: *" + "/" + "*\r\n" +
  "Content-Length: " + data.length() + "\r\n" +
  "Content-Type: application/x-www-form-urlencoded\r\n" +
  "\r\n" + data;
  
  String sendCmd = "AT+CIPSEND=";//determine the number of caracters to be sent.
  
  esp.print(sendCmd);
  esp.println(postRequest.length());
  
  delay(500);
  
  if(esp.find(">")) {
    Serial.println("Sending.."); 
    esp.print(postRequest);
  
    if( esp.find("SEND OK")) { 
      Serial.println("Packet sent");
    
      while (esp.available()) {
      
        String tmpResp = esp.readString();
        Serial.println(tmpResp);
      
      }
      
    // close the connection
      esp.println("AT+CIPCLOSE");
      
      }  
  }
}

void printTestValues(){
  Serial.print(" Humidity - " );
  Serial.println(dht.readHumidity());
  Serial.print(" Temparature - ");
  Serial.println(dht.readTemperature());
  Serial.print(" Light - ");
  Serial.println(measureLevelLight());
  Serial.print(" Door Status - ");
  Serial.println(getRelayStatus());
  Serial.print(" Power Status - ");
  Serial.println(getPowerStatus());

  Serial.println("--------------------------------- ");
}

String getSensorVals(){
  String locString = "";
  
  locString.concat("00,");
  locString.concat(imei);
  locString.concat(",");
  locString.concat(String(dht.readTemperature() - 5));
  locString.concat(",");
  locString.concat(String(dht.readHumidity() - 5));
  locString.concat(",");
  locString.concat(getPowerStatus());
  locString.concat(",");
  locString.concat(getRelayStatus());
  locString.concat(",");
  locString.concat(measureLevelLight());
  
  return locString;
}

void checkDoor(){

  doorState = getRelayStatus();

  if (doorState != lastDoorState) {
    if (doorState == HIGH) {
      doorStateCounter++;
      Serial.println("Closed");
      Serial.print("number door states: ");
      Serial.println(doorStateCounter);
      printTestValues();
//      payloadConstructor("01");
    } else {
      Serial.println("Open");
      printTestValues();
//      payloadConstructor("01");
    }
    delay(50);
  }
  lastDoorState = doorState;
  
}

void checkPower(){

    powerState = getPowerStatus();

  if (powerState != lastPowerState) {
    if (powerState == HIGH) {
      powerStateCounter++;
      Serial.println("On");
      Serial.print("number power switches: ");
      Serial.println(powerStateCounter);
      printTestValues();
    } else {
      printTestValues();
      Serial.println("Off");
    }
    delay(50);
  }
  lastPowerState = powerState;
  
}

void statusBlinker(){
  unsigned long ledloopT = millis();
  if ((unsigned long)(ledloopT - ledstartT) >= ledinterval){
    
    if (doorState == LOW && powerState == HIGH){
      
      blinkLED(13,3,200);
      
    } else if (doorState == HIGH && powerState == LOW ){
      
      blinkLED(13,2,300);
      
    } else if(powerState == LOW && doorState == LOW){

      blinkLED(13,6,100);
      
    }
    else {
      
      blinkLED(13,1,600);
      
    }
    
    delay(100);
    ledstartT = ledloopT;
  }
}


int measureLevelLight() {           
  return 1024-analogRead(lightPin); //Analogue value : 0 - Darkest : 1024 - Brightest
}

int getRelayStatus(){
  int doorChecker = digitalRead(reedPin);
  return doorChecker;  
}

int getPowerStatus(){
  int powerChecker = digitalRead(powerPin);
  return powerChecker;  
}

void blinkLED(int ledID, int repeat, int wait) {
  wdt_reset();
  if (repeat == 999) {
    digitalWrite(ledID, HIGH);
    return;
  }
  if (repeat == 0) {
    digitalWrite(ledID, LOW);
    return;
  }
  for (int i = 0; i < repeat; i++) {
    digitalWrite(ledID, HIGH);
    delay(wait);
    digitalWrite(ledID, LOW);
    delay(wait);
  }
  delay(1000);
}

boolean getMac(){
  String macCmd = "AT+CIPSTAMAC?";
  String id = "";

  esp.println(macCmd);

  while (esp.available()) {
    String tmpResp = esp.readStringUntil('\n');
//      Serial.println(tmpResp);
      if (tmpResp.substring(0,10) == "+CIPSTAMAC"){
        imei = tmpResp.substring(12,29);
        imei.trim();
//        Serial.println(imei);
        return true;
      } else {
        return false;
      }
  }
}
