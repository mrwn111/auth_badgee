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
#define BUTTON_PIN 7  // Bouton sur la broche D7

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MFRC522 mfrc522(SS_PIN, RST_PIN);

const char *ssid = "RouteurCadeau";
const char *motDePasseWifi = "CadeauRouteur";

const String userUID = "3614322998";
const String adminUID = "516457153";

unsigned long previousMillis = 0;
const long interval = 5000;
bool badgeDisplayed = false;

void setup()
{
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Configuration du bouton
  
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
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 20);
  display.println("Hello World");
  display.display();
}

void loop()
{
  unsigned long currentMillis = millis();
  int buttonState = digitalRead(BUTTON_PIN);

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
  {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("Le badge est :");
    display.display();

    String badgeUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      badgeUID += String(mfrc522.uid.uidByte[i], DEC);
    }

    if (buttonState == LOW) // Vérifier si le bouton est appuyé
    {
      if (badgeUID == userUID)
      {
        display.setCursor(0, 20);
        display.println("User");
        Serial.println("UID reconnu : User");
      }
      else if (badgeUID == adminUID)
      {
        display.setCursor(0, 20);
        display.println("Admin");
        Serial.println("UID reconnu : Admin");
      }
      else
      {
        display.setCursor(0, 20);
        display.println("Inconnu");
        Serial.println("UID inconnu");
      }
    }
    else
    {
      display.setCursor(0, 20);
      display.println("Appuyez sur le bouton!");
      Serial.println("Badge détecté mais bouton non pressé");
    }
    display.display();
    badgeDisplayed = true;
    previousMillis = currentMillis;
  }

  if (badgeDisplayed && (currentMillis - previousMillis >= interval))
  {
    display.clearDisplay();
    display.display();
    badgeDisplayed = false;
  }
}
