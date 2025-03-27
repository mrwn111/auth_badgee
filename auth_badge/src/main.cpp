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
#define Led_v 4

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MFRC522 mfrc522(SS_PIN, RST_PIN);

const char *ssid = "RouteurCadeau";
const char *motDePasseWifi = "CadeauRouteur";

// UIDs prédéfinis (en décimal sous forme de chaînes)
const String admin = "516457153";  // UID admin (en décimal)
const String user = "3614322998";  // UID user (en décimal)
const String doubleCheckUID = "1312364817";  // UID pour la double vérification

unsigned long previousMillis = 0;
const long interval = 5000;
bool badgeDisplayed = false;
unsigned long loadingMillis = 0;
int loadingState = 0;
bool waitingForSecondBadge = false;
unsigned long secondBadgeMillis = 0;
String firstBadgeUID = "";  // Stocke le premier UID (badge initial)
bool isProcessing = false;  // Variable de contrôle pour empêcher la relecture du badge pendant le processus

void setup()
{
  delay(1000);
  pinMode(Led_v, OUTPUT);
  digitalWrite(Led_v, LOW);

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

  // Si une carte est présente et nous ne sommes pas en train de traiter une autre vérification
  if (mfrc522.PICC_IsNewCardPresent() && !isProcessing)
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

      // Convertir l'UID en une chaîne décimale
      String uidString = "";
      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        uidString += String(mfrc522.uid.uidByte[i], DEC);
      }

      // Afficher l'UID en décimal sur le terminal
      Serial.print("UID du badge : ");
      Serial.println(uidString);

      // Si le badge est le badge de double vérification
      if (uidString == doubleCheckUID)
      {
        // Afficher "En attente de la 2e vérification" et attendre le second badge
        display.clearDisplay();
        display.setCursor(cursorX, cursorY);
        display.setTextSize(1);
        display.println("En attente de 2e verif");
        display.display();

        // Prépare le système pour attendre un second badge
        firstBadgeUID = uidString;  // Enregistrer le premier badge
        waitingForSecondBadge = true;
        secondBadgeMillis = currentMillis;
        digitalWrite(Led_v, LOW);  // Éteindre la LED en attendant le deuxième badge
        isProcessing = true;  // Début du processus de vérification, bloque les nouvelles cartes
      }
      // Si le badge est un admin ou user
      else if (uidString == admin || uidString == user)
      {
        // Les badges admin et user sont acceptés directement sans double vérification
        display.clearDisplay();
        display.setCursor(cursorX, cursorY);
        display.setTextSize(1);
        display.println(uidString == admin ? "Admin" : "User");
        display.display();
        digitalWrite(Led_v, HIGH);  // Allumer la LED pour un badge valide
        badgeDisplayed = true;
        previousMillis = currentMillis;
        isProcessing = true;  // Bloquer les nouvelles cartes jusqu'à ce que le processus soit terminé
      }
      else
      {
        // Badge inconnu
        display.clearDisplay();
        display.setCursor(cursorX, cursorY);
        display.setTextSize(1);
        display.println("Inconnu");
        display.display();
        digitalWrite(Led_v, LOW);
      }
    }
  }

  // Vérification du temps d'attente pour le deuxième badge (5 secondes)
  if (waitingForSecondBadge && (currentMillis - secondBadgeMillis >= 5000))
  {
    display.clearDisplay();
    display.setCursor(10, 30);
    display.setTextSize(1);
    display.println("Acces Refuse !");
    display.display();
    waitingForSecondBadge = false; // Réinitialisation
    digitalWrite(Led_v, LOW); // Éteindre la LED si l'accès est refusé
    isProcessing = false;  // Réinitialiser le processus pour permettre la lecture d'autres cartes
  }

  // Si on attend un second badge (après le badge "doubleCheckUID")
  if (waitingForSecondBadge && mfrc522.PICC_IsNewCardPresent())
  {
    if (mfrc522.PICC_ReadCardSerial())
    {
      String secondUidString = "";
      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        secondUidString += String(mfrc522.uid.uidByte[i], DEC);
      }

      // Vérification du second badge (admin ou user)
      if (secondUidString == admin || secondUidString == user)
      {
        display.clearDisplay();
        display.setCursor(10, 30);
        display.setTextSize(1);
        display.println("Acces Autorise");
        display.display();
        digitalWrite(Led_v, HIGH);  // LED allumée pour accès autorisé
        badgeDisplayed = true;
        previousMillis = currentMillis;
      }
      else
      {
        display.clearDisplay();
        display.setCursor(10, 30);
        display.setTextSize(1);
        display.println("Acces Refuse");
        display.display();
        digitalWrite(Led_v, LOW);  // LED éteinte pour accès refusé
      }

      waitingForSecondBadge = false; // Réinitialisation de l'attente du second badge
      isProcessing = false;  // Réinitialiser le processus pour permettre la lecture d'autres cartes
    }
  }

  // Si le badge est affiché pendant plus de 5 secondes, on réinitialise l'écran
  if (badgeDisplayed && (currentMillis - previousMillis >= interval))
  {
    display.clearDisplay();
    display.display();
    badgeDisplayed = false;
    digitalWrite(Led_v, LOW);
  }

  // Animation "En attente" sur l'écran si aucun badge n'est détecté
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
