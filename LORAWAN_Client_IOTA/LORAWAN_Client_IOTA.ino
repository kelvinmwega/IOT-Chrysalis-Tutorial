#include <TheThingsNetwork.h>
#include <SoftwareSerial.h>
#include <dht.h>

// Set your DevAddr, NwkSKey and AppSKey
const byte devAddr[4] = { 0x26, 0x01, 0x1A, 0x92 };
const byte nwkSKey[16] = { 0x20, 0x76, 0x8F, 0x80, 0xAF, 0x24, 0xC5, 0x4C, 0x17, 0xD3, 0x11, 0x49, 0x2E, 0x98, 0xE1, 0x61 };
const byte appSKey[16] = { 0xE6, 0x1A, 0x8C, 0x3B, 0xEC, 0x45, 0x71, 0x15, 0xA6, 0x25, 0x65, 0x12, 0x3A, 0x64, 0x96, 0x84 };

SoftwareSerial loraSerial(7, 8); // RX, TX
#define debugSerial Serial
dht DHT;
#define DHT11_PIN 2

TheThingsNetwork ttn(loraSerial, debugSerial, TTN_FP_EU868);

void setup() {
  loraSerial.begin(57600);
  debugSerial.begin(9600);

   //reset rn2483
  pinMode(10, OUTPUT);
  digitalWrite(10, LOW);
  delay(500);
  digitalWrite(10, HIGH);

  pinMode(13, OUTPUT);
  led_off();
  
  ttn.personalize(devAddr, nwkSKey, appSKey);
}

void loop() {
//  debugSerial.println("-- LOOP");
//  Serial.print("DHT11, \t");
  int chk = DHT.read11(DHT11_PIN);

  led_off();
  uint16_t temp = DHT.temperature*100;
  uint16_t hum = DHT.humidity*100;

  // Prepare payload of 1 byte to indicate LED status
  byte payload[4];
  payload[0] = highByte(temp);
  payload[1] = lowByte(temp);
  payload[2] = highByte(hum);
  payload[3] = lowByte(hum);

  // Send it off
  ttn.sendBytes(payload, sizeof(payload));
  led_on();
  delay(900000);
  
}

void led_on()
{
  digitalWrite(13, 1);
}

void led_off()
{
  digitalWrite(13, 0);
}
