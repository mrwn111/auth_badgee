#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define HOST_NAME "http://10.1.42.201:8000/"
#define HOST_PORT 8000

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

// Animation data
const unsigned char epd_bitmap_ezgif [] PROGMEM = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0x3f, 0xff, 0xff, 0xf8, 0x11, 0xff, 0xff, 0xf0, 0x00, 0x47, 0xff, 0xc0, 0x00, 0x00, 0xff, 
    0x80, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x0f, 0x80, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x03, 
    0x80, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 
    0xc0, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 
    0xfc, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 
    0xfc, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x01, 0xfc, 0x00, 0x00, 0x03, 
    0xfc, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 0xff, 0xff, 0xc0, 0x1f, 0xff
};

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 144)
const int epd_bitmap_allArray_LEN = 1;
const unsigned char* epd_bitmap_allArray[1] = {
    epd_bitmap_ezgif
};

// Setup the display and WiFi
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
  client.connect(HOST_NAME, HOST_PORT);

  unsigned long currentMillis = millis(); // Get current time

  if (mfrc522.PICC_IsNewCardPresent())
  {
    Serial.println("Test 1 passé");
    if (mfrc522.PICC_ReadCardSerial())
    {
      IPAddress myIP = WiFi.localIP();
      Serial.println(myIP);
      Serial.println("Test 2 passé");
      Serial.println("Nouveau Tag RFID :");
      Serial.print("RFID Tag (Decimal):");

      // Clear the display initially
      display.clearDisplay();

      // Display "Le badge est :" for 5 seconds
      display.setCursor(0, 0); // Position it near the top
      display.setTextSize(1);
      display.println("Le badge est :");
      display.display();

      // Set the previousMillis to currentMillis when the text is displayed
      previousMillis = currentMillis;
      badgeDisplayed = true; // Set flag that the badge text is displayed

      // Display the UID numbers
      int cursorX = 10; // Starting X position for UID
      int cursorY = 30; // Starting Y position below the "Le badge est :" text

      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        Serial.print(mfrc522.uid.uidByte[i], DEC);
        Serial.print(" ");

        // Convert the integer to a String
        String message = String(mfrc522.uid.uidByte[i], DEC);

        // Add space between digits by adjusting cursor
        display.setCursor(cursorX, cursorY);
        display.println(message);

        // Increase the X position for the next number
        cursorX += 25; // Increase space between numbers
      }

      display.display(); // Update the display with the UID
    }
  }

  // If 5 seconds have passed, clear the display and reset flag
  if (badgeDisplayed && (currentMillis - previousMillis >= interval))
  {
    // Clear the display (clear the badge and UID)
    display.clearDisplay();
    display.display();

    // Display animation after UID disappears
    for (int i = 0; i < epd_bitmap_allArray_LEN; i++)
    {
      // Draw the bitmap from PROGMEM (bitmap in array)
      display.clearDisplay();
      display.drawBitmap(0, 0, epd_bitmap_allArray[i], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
      display.display();
      delay(200); // Delay between frames to control animation speed
    }

    badgeDisplayed = false; // Reset the flag to stop further clearing
  }

  delay(100); // Reduce the delay for smoother operation
}
