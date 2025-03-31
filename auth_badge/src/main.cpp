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
#define NEW_BADGE_BUTTON_PIN 2

#define LED_VERTE_PIN 3
#define LED_ROUGE_PIN 4

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MFRC522 mfrc522(SS_PIN, RST_PIN);

const char *ssid = "RouteurCadeau";
const char *motDePasseWifi = "CadeauRouteur";
const char *token = "1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t";

const String apiURL = "http://10.1.42.201:8000/users/";

// UID du supresseur (doit être en dur dans le code)
String suppressorUID = "418416698";  // Exemple d'UID du supresseur

unsigned long previousMillis = 0;
const long interval = 5000;
bool badgeDisplayed = false;

unsigned long ledOffMillis = 0;
const long ledDuration = 3000;

void sendDeleteRequest(String badgeID)
{
  HTTPClient http;

  // URL de la requête DELETE
  String deleteBadgeURL = apiURL + badgeID;  // URL de suppression avec le badge ID

  http.begin(deleteBadgeURL);  // Démarrer la requête
  http.addHeader("X-API-Key", token);  // Ajouter l'API Key

  // Envoyer la requête DELETE
  int httpResponseCode = http.sendRequest("DELETE");  // Utiliser "DELETE" pour envoyer la requête

  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println("Réponse de la suppression du badge: " + response);
    display.clearDisplay();
    display.setCursor(0, 20);
    display.println("Badge supprimé!");
    display.display();
  }
  else
  {
    Serial.println("Erreur lors de la suppression du badge");
    display.clearDisplay();
    display.setCursor(0, 20);
    display.println("Erreur suppression");
    display.display();
  }

  http.end();  // Fermer la requête
}

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
  pinMode(NEW_BADGE_BUTTON_PIN, INPUT_PULLUP);
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

        // Vérifiez si c'est le badge du supresseur
        if (badgeUID == suppressorUID)
        {
          Serial.println("Badge du supresseur détecté. Aucune action effectuée.");
          display.clearDisplay();
          display.setCursor(0, 20);
          display.println("Badge supresseur!");
          display.display();
          return; // Ne rien faire si c'est le badge du supresseur
        }

        // Si ce n'est pas le badge du supresseur, on continue le processus de vérification
        HTTPClient http;

        String checkBadgeURL = apiURL + "check/" + badgeUID;
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
            display.println("Badge trouve!");
            display.println("Role: " + role);
            Serial.println("Badge trouve!");

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
  else if (digitalRead(NEW_BADGE_BUTTON_PIN) == LOW)
  {
    Serial.println("Entrez l'UID du nouveau badge (valeurs séparées par des espaces) : ");
    while (Serial.available() == 0);
    String badgeUID = Serial.readStringUntil('\n');
    badgeUID.trim();

    Serial.println("Entrez le mot de passe du nouveau badge : ");
    while (Serial.available() == 0);
    String password = Serial.readStringUntil('\n');
    password.trim();

    Serial.println("Entrez le rôle du nouveau badge (admin/user) : ");
    while (Serial.available() == 0);
    String role = Serial.readStringUntil('\n');
    role.trim();

    HTTPClient http;
    String addBadgeURL = "http://10.1.42.201:8000/users";
    http.begin(addBadgeURL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", token);

    DynamicJsonDocument doc(200);
    doc["badge_id"] = badgeUID;
    doc["password"] = password;
    doc["role"] = role;
    String jsonPayload;
    serializeJson(doc, jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0)
    {
      String response = http.getString();
      Serial.println("Réponse de l'API : " + response);
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println("Badge créé!");
      display.display();
    }
    else
    {
      Serial.println("Erreur lors de la création du badge");
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println("Erreur de creation");
      display.display();
    }

    http.end();
    delay(2000);
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
