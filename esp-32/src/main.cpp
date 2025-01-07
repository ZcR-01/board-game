#include <WiFi.h>
#include <PubSubClient.h>

// WiFi and MQTT configuration
const char *ssid = "Nothing_9181";
const char *password = "faraji12345";
const char *mqttServer = "192.168.72.222"; // Replace with your broker's IP address
const int mqttPort = 1883;
const char *mqttUser = "duo_hz";
const char *mqttPassword = "^D2E^%U2";

// MQTT client setup
WiFiClient espClient;
PubSubClient client(espClient);

// MQTT topic
const char *topicSubscribe = "players/+/actions/#";                 // Wildcard topic
const char *topicPublish = "players/esp32_base/actions/ready/base"; // Example of publish topic

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

  // Handle specific topic actions
  if (String(topic).startsWith("players/") && String(topic).endsWith("actions/moved"))
  {
    Serial.println("Action: Player moved detected");
  }
  else if (String(topic).startsWith("players/") && String(topic).endsWith("actions/shoot"))
  {
    Serial.println("Action: Player shoot detected");
  }
  else if (String(topic).startsWith("players/") && String(topic).endsWith("actions/die"))
  {
    Serial.println("Action: Player die detected");
  }
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

      // Subscribe to the wildcard topic
      client.subscribe(topicSubscribe);
      Serial.println("Subscribed to: " + String(topicSubscribe));

      // Publish readiness signal to the broker
      client.publish(topicPublish, "true");
      Serial.println("Base is ready - published readiness signal");
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

  // Ensure MQTT connection
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
