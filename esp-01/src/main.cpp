#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi and MQTT configuration
const char *ssid = "Nothing_9181";
const char *password = "faraji12345";
const char *mqttServer = "192.168.47.222";
const int mqttPort = 1883;
const char *mqttUser = "duo_hz";
const char *mqttPassword = "^D2E^%U2";

// MQTT client setup
WiFiClient espClient;
PubSubClient client(espClient);

// Meeple ID
const char *meepleId = "duo_hz";

// MQTT topics
const char *topicReady = "players/duo_hz/actions/ready/meeple";
const char *topicMove = "players/duo_hz/actions/move";
const char *topicDie = "players/duo_hz/actions/die";
const char *topicBulletState = "players/duo_hz/state/has_bullet";
const char *topicCanMove = "players/duo_hz/state/can_move";
const char *topicGameState = "state/stage";
const char *topicShoot = "players/duo_hz/actions/shoot";

// GPIO pins
const int hallSensorPin = 2; // Hall sensor for magnetic field detection
const int yellowLedPin = 0;  // Yellow LED for bullet indication
const int greenLedPin = 3;   // Green LED for movement status

// Game state variables
String currentGameState = "";
bool hasBullet = false;
bool canMove = false;

// Magnetic field state
bool previousMagnetState = false;
bool currentMagnetState = false;

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

// Function to make the yellow LED blink
void blinkYellowLed(int times, int duration)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(yellowLedPin, HIGH);
    delay(duration);
    digitalWrite(yellowLedPin, LOW);
    delay(duration);
  }
}

// Callback function for MQTT messages
void callback(char *topic, byte *payload, unsigned int length)
{
  String message = "";
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  // Handle game state changes
  if (strcmp(topic, topicGameState) == 0)
  {
    currentGameState = message;
    Serial.print("Game state changed to: ");
    Serial.println(currentGameState);
  }
  else if (strcmp(topic, topicBulletState) == 0)
  {
    hasBullet = (message == "True");
    digitalWrite(yellowLedPin, hasBullet ? HIGH : LOW);
    Serial.println(hasBullet ? "Meeple has the bullet" : "Meeple lost the bullet");
  }
  else if (strcmp(topic, topicCanMove) == 0)
  {
    canMove = (message == "True");
    digitalWrite(greenLedPin, canMove ? HIGH : LOW);
    Serial.println(canMove ? "Meeple can move" : "Meeple cannot move");
  }
  else if (strcmp(topic, topicShoot) == 0)
  {
    blinkYellowLed(3, 500);
  }
}

// Function to connect to MQTT broker
void reconnectMQTT()
{
  while (!client.connected())
  {
    Serial.print("Connecting to MQTT...");
    if (client.connect(meepleId, mqttUser, mqttPassword))
    {
      Serial.println("connected");
      client.subscribe(topicGameState);
      client.subscribe(topicBulletState);
      client.subscribe(topicCanMove);

      // Publish readiness to the master
      if (client.publish(topicReady, "True"))
      {
        Serial.println("Ready signal published successfully.");
      }
      else
      {
        Serial.println("Failed to publish ready signal.");
      }

      Serial.println("Meeple ready signal sent");
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

// Detect movement when the magnetic field disconnects and reconnects
void detectMovement()
{
  if (currentGameState == "moving" && canMove)
  {
    currentMagnetState = digitalRead(hallSensorPin) == LOW; // LOW = magnet detected

    // Detect transition from no magnet to magnet (indicates a movement)
    if (!previousMagnetState && currentMagnetState)
    {
      client.publish(topicMove, "True");
      Serial.println("Movement detected and reported.");
    }

    // Update previous state
    previousMagnetState = currentMagnetState;
  }
}

// Detect death if the magnetic field disconnects completely in the shooting phase
void detectDeath()
{
  if (currentGameState == "shooting")
  {
    currentMagnetState = digitalRead(hallSensorPin) == LOW;

    // If magnet is not detected at all, report death
    if (!currentMagnetState)
    {
      client.publish(topicDie, "True");
      Serial.println("Meeple has died (no magnetic field).");
      delay(500); // Prevent spamming the topic
    }
  }
}

// Setup function
void setup()
{
  Serial.begin(115200);
  pinMode(hallSensorPin, INPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);

  // Initial states
  digitalWrite(yellowLedPin, LOW);
  digitalWrite(greenLedPin, LOW);

  setupWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  reconnectMQTT();
}

// Main loop
void loop()
{
  if (!client.connected())
  {
    reconnectMQTT();
  }
  client.loop();

  // Detect and report movement
  detectMovement();

  // Detect death in shooting phase
  detectDeath();
}
