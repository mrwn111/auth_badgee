#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> 

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define RST_PIN 5
#define SS_PIN 21
#define BUTTON_PIN 20

#define LED_VERTE_PIN 3  
#define LED_ROUGE_PIN 4  

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MFRC522 mfrc522(SS_PIN, RST_PIN);

const char *ssid = "RouteurCadeau";
const char *motDePasseWifi = "CadeauRouteur";
const char *token = "1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t"; 

const String apiURL = "http://10.1.42.201:8000/users/check/";

unsigned long previousMillis = 0;
const long interval = 5000;
bool badgeDisplayed = false;

unsigned long ledOffMillis = 0; 
const long ledDuration = 3000; 

void setup()
{
  delay(1000);
  Serial.begin(115200);
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

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_VERTE_PIN, OUTPUT);
  pinMode(LED_ROUGE_PIN, OUTPUT);  

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

void loop()
{
  unsigned long currentMillis = millis();

  if (digitalRead(BUTTON_PIN) == LOW)
  {
    if (mfrc522.PICC_IsNewCardPresent())
    {
      if (mfrc522.PICC_ReadCardSerial())
      {
        display.clearDisplay();

        String badgeUID = "";
        for (byte i = 0; i < mfrc522.uid.size; i++)
        {
          badgeUID += String(mfrc522.uid.uidByte[i], DEC);
        }

        display.setCursor(0, 0);
        display.setTextSize(1);
        display.println("Verification...");
        display.display();

        Serial.println(badgeUID); 

        HTTPClient http;
        
        String checkBadgeURL = apiURL + badgeUID;
        http.begin(checkBadgeURL);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("X-API-Key", token);

        int httpResponseCode = http.GET();

        if (httpResponseCode > 0)
        {
          String checkResponse = http.getString();
          Serial.println("Réponse de la vérification du badge: " + checkResponse);

          DynamicJsonDocument doc(200); 
          DeserializationError error = deserializeJson(doc, checkResponse);

          if (error)
          {
            Serial.println("Erreur lors de l'analyse de la réponse JSON");
            return;
          }

          bool exists = doc["exists"];
          String role = doc["user_info"]["role"];

          display.clearDisplay();
          display.setCursor(0, 20);
          display.setTextSize(1);

          if (exists)
          {
            display.println("Badge trouvé!");
            display.println("Rôle: " + role);
            Serial.println("Badge trouvé!");

            if (role == "admin")
            {
              digitalWrite(LED_VERTE_PIN, HIGH); 
              digitalWrite(LED_ROUGE_PIN, LOW);   
            }
            else if (role == "user")
            {
              digitalWrite(LED_VERTE_PIN, HIGH); 
              digitalWrite(LED_ROUGE_PIN, LOW);   
            }
            else
            {
              digitalWrite(LED_VERTE_PIN, LOW);   
              digitalWrite(LED_ROUGE_PIN, HIGH); 
            }
          }
          else
          {
            display.println("Badge introuvable");
            Serial.println("Badge introuvable");
            digitalWrite(LED_VERTE_PIN, LOW);   
            digitalWrite(LED_ROUGE_PIN, HIGH);  
          }
          ledOffMillis = millis() + ledDuration; 
          display.display();
        }
        else
        {
          Serial.println("Erreur lors de la vérification du badge");
          display.clearDisplay();
          display.setCursor(0, 20);
          display.println("Erreur de vérification");
          display.display();
          digitalWrite(LED_VERTE_PIN, LOW);  
          digitalWrite(LED_ROUGE_PIN, HIGH);  
          ledOffMillis = millis() + ledDuration; 
        }

        http.end();

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
  }
  else
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, SCREEN_HEIGHT / 2);
    display.println("Appuyez sur le bouton");
    display.println("pour badger");
    display.display();
  }

  if (millis() > ledOffMillis)
  {
    digitalWrite(LED_VERTE_PIN, LOW);  
    digitalWrite(LED_ROUGE_PIN, LOW); 
  }

  delay(100);
}
