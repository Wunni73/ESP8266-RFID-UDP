/*
 * ----------------------------------------------------------------------
 * Program to read NUID from a PICC to serial and via UDP (WiFi)
 * ----------------------------------------------------------------------
 * Wunni
 * 
 * RC522 Interfacing with NodeMCU / ESP8266 / ESP D1mini
 * 
 * Typical pin layout used:
 * ----------------------------------
 *             MFRC522      Node     
 *             Reader/PCD   MCU            ESP8266    D1mini    
 * Signal      Pin          Pin      
 * ----------------------------------         
 * RST/Reset   RST          D1 (GPIO5)        D4        D0
 * SPI SS      SDA(SS)      D2 (GPIO4)        D8        D8
 * SPI MOSI    MOSI         D7 (GPIO13)
 * SPI MISO    MISO         D6 (GPIO12)
 * SPI SCK     SCK          D5 (GPIO14)
 * 3.3V        3.3V         3.3V
 * GND         GND          GND
 */

#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "???????";           // ?SSID
const char* password = "???????";       // ?Pasword
//constexpr uint8_t RST_PIN = 5;         // Configurable, see typical pin layout above
//constexpr uint8_t SS_PIN = 4;           // Configurable, see typical pin layout above
#define RST_PIN         D0                   // Configurable, see typical pin layout above
#define SS_PIN          D8                    // Configurable, see typical pin layout above
MFRC522 rfid(SS_PIN, RST_PIN);      // Instance of the class
unsigned long cardId = 0;

//#define LED_RED D0 //D3
//#define LED_GREEN D3 //D4  D0

WiFiUDP Udp;
unsigned int localUdpPort = 7002;  // local port to listen on


void setup()
{
  Serial.begin(115200);
  SPI.begin(); // Init SPI bus
  // LED ansteuerung
  pinMode(LED_BUILTIN, OUTPUT);
 digitalWrite(LED_BUILTIN, HIGH);
//  pinMode(LED_GREEN, OUTPUT);         
//  pinMode(LED_RED, OUTPUT);
//  digitalWrite(LED_GREEN, LOW);
//  digitalWrite(LED_RED, LOW);
  
  rfid.PCD_Init(); // Init MFRC522 

  Serial.println("");
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
// digitalWrite(LED_GREEN, HIGH);
digitalWrite(LED_BUILTIN, LOW);
  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
}


void loop()
{


  // Look for new cards
  if ( ! rfid.PICC_IsNewCardPresent()) {
    return;
  }
  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial()) {
    return;
  }
  cardId = getCardId();

  Serial.print("New ");
  Serial.println(cardId);

  char buffer[10];
  sprintf(buffer, "%lu", cardId);
    Udp.beginPacket("192.168.178.65", 1234);            // IP and port of the LOXONE Miniserver
//    Udp.write("Play");
    Udp.write(buffer);
    Udp.endPacket();
  Serial.println("Card");
//   digitalWrite(LED_RED, LOW);
 digitalWrite(LED_BUILTIN, HIGH);
  uint8_t control = 0x00;

  do {
    control = 0;
    for (int i = 0; i < 3; i++) {
      if (!rfid.PICC_IsNewCardPresent()) {
        if (rfid.PICC_ReadCardSerial()) {
          control |= 0x16;
        }

        if (rfid.PICC_ReadCardSerial()) {
          control |= 0x16;
        }

        control += 0x1;
      }

      control += 0x4;
    }

    delay(0);
  } while (control == 13 || control == 14);


  Serial.println("Removed");
//  digitalWrite(LED_RED, HIGH);
digitalWrite(LED_BUILTIN, LOW);
    Udp.beginPacket("192.168.178.65", 1234);            // IP and port of the LOXONE Miniserver
    Udp.write("Stop");
    Udp.endPacket();
  delay(500);

// Halt PICC
  rfid.PICC_HaltA();

// Stop encryption on PCD
  rfid.PCD_StopCrypto1();
//   digitalWrite(LED_GREEN, LOW);
//   digitalWrite(LED_RED, LOW);
}

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
unsigned long getCardId() {
  byte readCard[4];
  for (int i = 0; i < 4; i++) {
    readCard[i] = rfid.uid.uidByte[i];
  }

  return (unsigned long)readCard[0] << 24
    | (unsigned long)readCard[1] << 16
    | (unsigned long)readCard[2] << 8
    | (unsigned long)readCard[3];
}
