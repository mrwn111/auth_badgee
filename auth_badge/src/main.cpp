#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define RST_PIN 5
#define SS_PIN 21

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MFRC522 mfrc522(SS_PIN, RST_PIN);

const char *ssid = "RouteurCadeau";
const char *motDePasseWifi = "CadeauRouteur";

unsigned long previousMillis = 0; 
const long interval = 5000;      
bool badgeDisplayed = false;    
unsigned long loadingMillis = 0;  
int loadingState = 0;            
void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial.println("Connexion au réseau WiFi en cours");
  WiFi.begin(ssid, motDePasseWifi);
  WiFi.softAP(ssid, motDePasseWifi);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.write('.'); 
  }
  Serial.println("\nWiFi connecté");

  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("Place une carte sur le RFID");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  String message = "Hello World";
  int textWidth = message.length() * 6;
  int cursorX = (SCREEN_WIDTH - textWidth) / 2;
  display.setCursor(cursorX, 20);
  display.println(message);
  display.display();
}

void loop()
{
  WiFiClient client;

  unsigned long currentMillis = millis(); 

  if (mfrc522.PICC_IsNewCardPresent())
  {
    if (mfrc522.PICC_ReadCardSerial())
    {
      display.clearDisplay();

      display.setCursor(0, 0); 
      display.setTextSize(1);
      display.println("Le badge est :");
      display.display();

      int cursorX = 10; 
      int cursorY = 30; 

      Serial.print("UID du badge : ");
      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        String message = String(mfrc522.uid.uidByte[i], DEC);

        display.setCursor(cursorX, cursorY);
        display.println(message);

        cursorX += 25;

        Serial.print(mfrc522.uid.uidByte[i], DEC);
        Serial.print(" ");
      }
      Serial.println(); 

      display.display();
      
      badgeDisplayed = true;
      previousMillis = currentMillis;
    }
  }

  if (badgeDisplayed && (currentMillis - previousMillis >= interval))
  {
    display.clearDisplay();
    display.display();

    badgeDisplayed = false;
  }

  if (!badgeDisplayed)
  {
    if (currentMillis - loadingMillis >= 500)
    {
      loadingMillis = currentMillis;

      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(30, SCREEN_HEIGHT / 2); 

      if (loadingState == 0)
      {
        display.println("En attente");
      }
      else if (loadingState == 1)
      {
        display.println("En attente.");
      }
      else if (loadingState == 2)
      {
        display.println("En attente..");
      }
      else if (loadingState == 3)
      {
        display.println("En attente..:)");
      }
      else if (loadingState == 4)
      {
        display.println("En attente..");
      }
      else if (loadingState == 5)
      {
        display.println("En attente.");
      }


      display.display();

      loadingState = (loadingState + 1) % 6; 
    }
  }

  delay(100); 
}
