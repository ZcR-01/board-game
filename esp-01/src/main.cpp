#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi and MQTT configuration
const char *ssid = "MiFibra-CE8F";
const char *password = "KCiX33qi";
const char *mqttServer = "192.168.1.x"; // Replace with your broker's IP address
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
  for (int i = 0; i < length; i++)
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
      blinkGreenLed();
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
