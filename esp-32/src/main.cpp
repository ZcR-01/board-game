#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi and MQTT configuration
const char *ssid = "Nothing_9181";
const char *password = "faraji12345";
const char *mqttServer = "agrospai.udl.cat"; // Replace with your broker's IP address
const int mqttPort = 1883;
const char *mqttUser = "duo_hz";
const char *mqttPassword = "^D2E^%U2";

// MQTT client setup
WiFiClient espClient;
PubSubClient client(espClient);

// Unique Base ID
const char *baseId = "duo_hz_base";

// MQTT topics
const char *topicSubscribeState = "state/stage";
const char *topicPublishShoot = "players/duo_hz/actions/shoot";
const char *topicPublishReady = "players/duo_hz/actions/ready/base";
const char *topicSubscribeBullet = "players/duo_hz/state/has_bullet";
const char *topicSubscribeDied = "players/duo_hz/state/has_died"; // Subscribe to death topic (careful with player and no players)
const char *topicSubscribeMoved = "players/duo_hz/actions/move";
const char *topicSubscribeWin = "players/duo_hz/state/has_won";
const char *topicSubscribeDie = "players/duo_hz/actions/die";

// Hardware pins
#define BUZZER_PIN 12
#define BUTTON_PIN 25
#define SDA_PIN 33
#define SCL_PIN 32

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2); // Change address if needed

// Game state
String gameStage = "joining";
bool hasBullet = false;
bool isAlive = true;
bool buttonPressed = false;
bool decisionTaken = false;

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
  Serial.println(WiFi.localIP());
}

// Victory buzzer function
void playVictoryTone()
{
  int melody[] = {262, 294, 330, 349, 392, 440, 494, 523}; // Simple scale
  int duration = 200;                                      // Duration of each note

  for (int i = 0; i < 8; i++)
  {
    tone(BUZZER_PIN, melody[i], duration);
    delay(duration + 50); // Small pause between notes
  }
}

// Death buzzer function
void playDeathTone()
{
  for (int i = 0; i < 3; i++)
  {
    tone(BUZZER_PIN, 200, 500); // Low-pitched tone
    delay(600);
  }
}

// Callback function for MQTT messages
void callback(char *topic, byte *payload, unsigned int length)
{
  String message = "";
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  Serial.printf("Message received on topic: %s, Message: %s\n", topic, message.c_str());

  // Handle game state updates
  if (strcmp(topic, topicSubscribeState) == 0)
  {
    gameStage = message;
    lcd.setCursor(0, 0);
    lcd.print("Stage: ");
    lcd.print(gameStage);
    tone(BUZZER_PIN, 1000, 200); // Notify with a beep

    if (gameStage != "shooting")
    {
      decisionTaken = false;
    }
  }

  // Handle Bullet state
  if (strcmp(topic, topicSubscribeBullet) == 0)
  {
    hasBullet = (message == "True");
    lcd.setCursor(0, 1);
    lcd.print("Bullet: ");
    lcd.print(hasBullet ? "YES " : "NO  ");
  }

  // Handle death state
  if (strcmp(topic, topicSubscribeDied) == 0)
  {
    isAlive = (message != "True");
    if (!isAlive)
    {
      lcd.setCursor(0, 0);
      lcd.print("You are DEAD      ");
      tone(BUZZER_PIN, 200, 1000); // Long beep for death
      Serial.println("You are dead");
      delay(3000);
    }
  }

  // Handle die action
  if (strcmp(topic, topicSubscribeDie) == 0)
  {
    if (message == "True")
    {
      lcd.setCursor(0, 0);
      lcd.print("Action: You Die   ");
      Serial.println("Die action triggered");
      playDeathTone(); // Play death tone
    }
  }

  // Handle movement detection
  if (strcmp(topic, topicSubscribeMoved) == 0)
  {
    if (message == "True")
    {
      lcd.setCursor(0, 1);
      lcd.print("Moved Detected    ");
      tone(BUZZER_PIN, 800, 100); // Notify movement
    }
  }

  // Handle winning state
  if (strcmp(topic, topicSubscribeWin) == 0)
  {
    if (message == "True")
    {
      lcd.setCursor(0, 0);
      lcd.print("YOU WIN!         ");
      Serial.println("Player has won");
      playVictoryTone(); // Play victory tone
    }
  }
}

// Function to connect to MQTT broker
void reconnectMQTT()
{
  while (!client.connected())
  {
    Serial.print("Connecting to MQTT...");
    if (client.connect(baseId, mqttUser, mqttPassword))
    {
      Serial.println("connected");

      // Subscribe to topics
      client.subscribe(topicSubscribeState);
      client.subscribe(topicSubscribeBullet);
      client.subscribe(topicSubscribeDied);
      client.subscribe(topicSubscribeMoved); // Subscribe to movement detection topic
      client.subscribe(topicSubscribeWin);   // Subscribe to win state topic
      client.subscribe(topicSubscribeDie);
      Serial.println("Subscribed to necessary topics");

      // Publish readiness signal
      client.publish(topicPublishReady, "True");
      lcd.setCursor(0, 0);
      lcd.print("Ready for game!");
      tone(BUZZER_PIN, 1500, 500); // Notify with a beep
      delay(1000);
    }
    else
    {
      Serial.printf("failed, rc=%d\n", client.state());
      delay(5000);
    }
  }
}

// Setup function
void setup()
{
  Serial.begin(115200);

  // Initialize hardware
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.print("Initializing...");

  // Connect to WiFi
  setupWiFi();

  // Initialize MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // Ensure MQTT connection
  reconnectMQTT();
}

// Check button press
void handleButton()
{
  static bool lastState = HIGH;
  bool currentState = digitalRead(BUTTON_PIN);

  if (lastState == HIGH && currentState == LOW)
  {
    buttonPressed = true;
    Serial.println("Button pressed");
  }
  lastState = currentState;
}

void handleOperatorDecision()
{
  if (gameStage == "shooting" && hasBullet && isAlive && !decisionTaken)
  {
    if (buttonPressed)
    {
      client.publish(topicPublishShoot, "True");
      lcd.setCursor(0, 1);
      lcd.print("Decision: Shoot");
      tone(BUZZER_PIN, 1000, 200); // Beep for shoot
      Serial.println("Decision: Shoot published");
      decisionTaken = true;
      buttonPressed = false;
    }
  }
}

// Main loop
void loop()
{
  if (!client.connected())
  {
    reconnectMQTT();
  }
  client.loop();

  handleButton();
  handleOperatorDecision();

  delay(100);
}
