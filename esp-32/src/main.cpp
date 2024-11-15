#include <WiFi.h>
#include <PubSubClient.h>

// WiFi and MQTT configuration
const char *ssid = "DIGIFIBRA-C40E";
const char *password = "AU7RPU48TJ";
const char *mqttServer = "192.168.1.157"; // Replace with your broker's IP address
const int mqttPort = 1883;
const char *mqttUser = "duo_hz";
const char *mqttPassword = "^D2E^%U2";

// MQTT client setup
WiFiClient espClient;
PubSubClient client(espClient);

// MQTT topics
const char *topicReadyBase = "/ready/base";

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
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Function to connect to MQTT broker
void reconnectMQTT()
{
  while (!client.connected())
  {
    Serial.print("Connecting to MQTT...");
    if (client.connect("BaseESP32", mqttUser, mqttPassword))
    {
      Serial.println("connected");
      client.subscribe(topicReadyBase);

      // Send readiness signal to the broker
      client.publish(topicReadyBase, "true");
      Serial.println("Base is ready - published /ready/base: true");
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

// Setup function
void setup()
{
  Serial.begin(115200);
  setupWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // Ensure MQTT connection and publish readiness
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
}
