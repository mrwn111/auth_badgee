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

unsigned long previousMillis = 0; // Store the last time the text was displayed
const long interval = 5000;       // 5 seconds interval (5000 milliseconds)
bool badgeDisplayed = false;      // Flag to track if the badge text was displayed

unsigned long loadingMillis = 0;  // Store the last time the "En attente" message was updated
int loadingState = 0;             // Used to cycle through "En attente", "En attente.", "En attente..", "En attente..."

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
    Serial.write('.');  // Show dots while waiting for connection
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

  // Display initial "Hello World"
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

  unsigned long currentMillis = millis(); // Get current time

  if (mfrc522.PICC_IsNewCardPresent())
  {
    if (mfrc522.PICC_ReadCardSerial())
    {
      // Clear the display initially
      display.clearDisplay();

      // Display "Le badge est :"
      display.setCursor(0, 0); 
      display.setTextSize(1);
      display.println("Le badge est :");
      display.display();

      // Display the UID numbers and print to serial in decimal format
      int cursorX = 10; 
      int cursorY = 30; 

      Serial.print("UID du badge : ");
      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        String message = String(mfrc522.uid.uidByte[i], DEC);

        display.setCursor(cursorX, cursorY);
        display.println(message);

        cursorX += 25;

        // Serial print each byte in decimal
        Serial.print(mfrc522.uid.uidByte[i], DEC);
        Serial.print(" ");
      }
      Serial.println();  // Newline after UID print to serial

      display.display();
      
      // Set the flag to indicate the badge has been displayed
      badgeDisplayed = true;
      previousMillis = currentMillis; // Store the current time to start the 5-second countdown
    }
  }

  // If 5 seconds have passed since displaying the badge, clear the display
  if (badgeDisplayed && (currentMillis - previousMillis >= interval))
  {
    // Clear the display (clear the badge and UID)
    display.clearDisplay();
    display.display();

    // Reset flag so the display doesn't keep clearing
    badgeDisplayed = false;
  }

  // If no badge text is displayed, show "Loading..." or "En attente"
  if (!badgeDisplayed)
  {
    // Update the "En attente" message every 500 milliseconds
    if (currentMillis - loadingMillis >= 500)
    {
      loadingMillis = currentMillis;  // Update the last time the message was updated

      // Clear the display and show the "En attente" message
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(30, SCREEN_HEIGHT / 2);  // Center the "En attente" text vertically

      // Cycle through "En attente", "En attente.", "En attente..", "En attente..."
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

      // Update the loading state for the next cycle
      loadingState = (loadingState + 1) % 6;  // Cycle between 0, 1, 2, 3
    }
  }

  delay(100); // Reduce the delay for smoother operation
}
