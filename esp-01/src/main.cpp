#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi and MQTT configuration
const char *ssid = "Nothing_9181";
const char *password = "faraji12345";
const char *mqttServer = "192.168.56.1"; // Replace with your broker's IP address
const int mqttPort = 1883;
const char *mqttUser = "duo_hz";
const char *mqttPassword = "^D2E^%U2";

// MQTT client setup
WiFiClient espClient;
PubSubClient client(espClient);

// MQTT topics
const char *topicReady = "/players/duo_hz/actions/ready/meeple";
const char *topicMove = "/players/duo_hz/actions/moved";
const char *topicDie = "/players/duo_hz/actions/die";
const char *topicShoot = "/players/duo_hz/actions/shoot";
const char *topicBulletState = "/players/duo_hz/state/has_bullet";
const char *topicCanMove = "/players/duo_hz/state/can_move";
const char *topicGameState = "/state/stage";

// GPIO pins
const int hallSensorPin = 2; // Hall sensor for magnetic field detection
const int yellowLedPin = 0;  // Yellow LED for bullet indication
const int greenLedPin = 3;   // Green LED for movement status

// Game state
String currentGameState = "";

// Function to connect to WiFi
void setupWiFi()
{
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

// Callback function for MQTT messages
void callback(char *topic, byte *payload, unsigned int length)
{
  String message = "";
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  if (String(topic) == topicBulletState)
  {
    bool hasBullet = (message == "true");
    digitalWrite(yellowLedPin, hasBullet ? HIGH : LOW);
  }
  else if (String(topic) == topicCanMove)
  {
    bool canMove = (message == "true");
    if (canMove && currentGameState == "moving")
    {
      Serial.println("Movement phase - Can move");
    }
  }
  else if (String(topic) == topicGameState)
  {
    currentGameState = message;
    Serial.print("Game state changed to: ");
    Serial.println(currentGameState);
  }
}

// Function to connect to MQTT broker
void reconnectMQTT()
{
  while (!client.connected())
  {
    Serial.print("Connecting to MQTT...");
    if (client.connect("MeepleESP", mqttUser, mqttPassword))
    {
      Serial.println("connected");
      client.subscribe(topicBulletState);
      client.subscribe(topicCanMove);
      client.subscribe(topicGameState);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Function for LED indication
void blinkGreenLed()
{
  digitalWrite(greenLedPin, HIGH);
  delay(200);
  digitalWrite(greenLedPin, LOW);
  delay(200);
}

// Setup function
void setup()
{
  pinMode(hallSensorPin, INPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);

  Serial.begin(115200);
  setupWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

// Main loop
void loop()
{
  if (!client.connected())
  {
    reconnectMQTT();
  }
  client.loop();

  // Check hall sensor for movement detection if the game state is "moving"
  if (currentGameState == "moving")
  {
    int hallSensorState = digitalRead(hallSensorPin);
    if (hallSensorState == LOW)
    {
      Serial.println("Magnet detected - reporting movement");
      client.publish(topicMove, "true");
    }
    else
    {
      Serial.println("No magnet detected");
    }
  }

  // Handle other states as needed
  if (currentGameState == "shooting")
  {
    Serial.println("Shooting phase - waiting for shoot command");
  }
  else if (currentGameState == "end")
  {
    Serial.println("Game has ended");
  }
}

// Buzzer + button

// #define BUTTON_PIN 25 // ESP32 GPIO pin to which the button is connected
// #define BUZZER_PIN 12

// #include "pitches.h"

// // notes in the melody:
// int melody[] = {
//     NOTE_E5, NOTE_E5, NOTE_E5,
//     NOTE_E5, NOTE_E5, NOTE_E5,
//     NOTE_E5, NOTE_G5, NOTE_C5, NOTE_D5,
//     NOTE_E5,
//     NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5,
//     NOTE_F5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5,
//     NOTE_E5, NOTE_D5, NOTE_D5, NOTE_E5,
//     NOTE_D5, NOTE_G5};

// // note durations: 4 = quarter note, 8 = eighth note, etc, also called tempo:
// int noteDurations[] = {
//     8, 8, 4,
//     8, 8, 4,
//     8, 8, 8, 8,
//     2,
//     8, 8, 8, 8,
//     8, 8, 8, 16, 16,
//     8, 8, 8, 8,
//     4, 4};

// void buzzer();

// void setup()
// {
//   Serial.begin(115200);              // initialize serial
//   pinMode(BUTTON_PIN, INPUT_PULLUP); // set ESP32 pin to input mode with pull-up resistor
//   pinMode(BUZZER_PIN, OUTPUT);       // set ESP32 pin to output mode
//   Serial.println("Press the button to play the melody");
// }

// void loop()
// {
//   if (digitalRead(BUTTON_PIN) == LOW)
//   { // button is pressed
//     Serial.println("The button is being pressed");
//     buzzer();
//   }
// }

// void buzzer()
// {
//   // iterate over the notes of the melody:
//   int size = sizeof(noteDurations) / sizeof(int);

//   for (int thisNote = 0; thisNote < size; thisNote++)
//   {
//     // to calculate the note duration, take one second divided by the note type.
//     // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
//     int noteDuration = 1000 / noteDurations[thisNote];
//     tone(BUZZER_PIN, melody[thisNote], noteDuration);

//     // to distinguish the notes, set a minimum time between them.
//     // the note's duration + 30% seems to work well:
//     int pauseBetweenNotes = noteDuration * 1.30;
//     delay(pauseBetweenNotes);
//     // stop the tone playing:
//     noTone(BUZZER_PIN);
//   }
// }

// LCD Screen

// #include <Arduino.h>
// #include <LiquidCrystal_I2C.h>
// #include <Wire.h>

// // set the LCD number of columns and rows
// int lcdColumns = 16;
// int lcdRows = 2;

// // set LCD address, number of columns and rows
// // if you don't know your display address, run an I2C scanner sketch
// LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// void setup()
// {
//   Wire.begin(33, 32);
//   // initialize LCD
//   lcd.init();
//   // turn on LCD backlight
//   lcd.backlight();
// }

// void loop()
// {
//   // set cursor to first column, first row
//   lcd.setCursor(0, 0);
//   // print message
//   lcd.print("Hello, World!");
//   delay(1000);
//   // clears the display to print new message
//   lcd.clear();
//   // set cursor to first column, second row
//   lcd.setCursor(0, 1);
//   lcd.print("Hello, World!");
//   delay(1000);
//   lcd.clear();
// }