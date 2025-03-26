#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 5
#define SS_PIN 6

MFRC522 mfrc522(SS_PIN, RST_PIN); 

void setup() {
  Serial.begin(115200); 
  SPI.begin(); 

  mfrc522.PCD_Init(); 

  Serial.println("Place une carte sur le RFID");
}

void loop() {
  if (mfrc522.PICC_IsNewCardPresent()) {
    Serial.println("Test 1 passé");
    if (mfrc522.PICC_ReadCardSerial()) {
      Serial.println("Test 2 passé");
      Serial.println("Nouveau Tag RFID :");
      Serial.print("RFID Tag :");

      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
      }
      Serial.println();

      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(300);
    }
  }
  delay(1000);
}
