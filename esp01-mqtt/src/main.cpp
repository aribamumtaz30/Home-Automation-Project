#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// #define LED 2
// #define WDT_TIMEOUT 20000

// WiFi credentials
const char *ssid = "Home Base";
const char *password = "leb87/11";

// MQTT Broker
const char *mqtt_server = "emqx.home.mwaleedh.com.pk";
const int mqtt_port = 8883; // MQTT port
const char *mqtt_user = "test";
const char *mqtt_password = "test";
const char *mqtt_client_id = "esp01_rameeza";
bool switchState = false; // Initial state
float temperature = 0.0;  // Initial temperature value

WiFiClientSecure espClient;
PubSubClient client(espClient);

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("MQTT connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retry in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  // Parse JSON payload
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  // digitalWrite(LED, LOW);

  // Extract values from JSON and update variables
  switchState = doc["switch"];
  temperature = doc["temperature"];

  Serial.print("Switch state: ");
  Serial.println(switchState);
  Serial.print("Temperature: ");
  Serial.println(temperature);
  delay(1000);
}
unsigned long previousMillis = 0;

void nonBlockingDelay(unsigned long interval) {
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    // Perform action here...
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Turning On...");
  delay(5000);

  WiFi.begin(ssid, password);
  Serial.println("Wifi Function called");
  while (WiFi.status() != WL_CONNECTED) {
    nonBlockingDelay(50000);
    Serial.println("Connecting to WiFi...");
    // delay(5000);
  }
  Serial.println("WiFi connected");

  // if (root_ca != NULL) {
  //   espClient.setCACert(root_ca);
  // } else {
  espClient.setInsecure();
  //}
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("MQTT connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retry in 5 seconds");
      delay(5000);
    }
  }
  // send initial config message here, intial subscribe here
  // client.subscribe("topic_name");
  // Serial.println("Subscribed to topic");
  // client.publish(topic.c_str(), message.c_str());

  // action config here
  // pinMode(LED, OUTPUT);
}

void loop() {
  // ESP.wdtFeed();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // digitalWrite(LED, HIGH);
  //  Read switch state and temperature from serial
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    if (input.startsWith("publish:")) {
      int separatorIndex = input.indexOf('|');
      if (separatorIndex != -1) {
        String topic = input.substring(8, separatorIndex);
        String payload = input.substring(separatorIndex + 1);
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          switchState = doc["switch"];
          temperature = doc["temperature"];
          client.publish(topic.c_str(), payload.c_str());
          Serial.println("Published message:");
          Serial.println(payload);
        }
        //   // char buffer[256];
        //   // size_t n = serializeJson(doc, buffer);
      }
    } else if (input.startsWith("subscribe:")) {
      String topic = input.substring(10);
      client.subscribe(topic.c_str());
      Serial.print("Subscribed to topic: ");
      Serial.println(topic);
    } else if (input.startsWith("unsubscribe:")) {
      String topic = input.substring(12);
      client.unsubscribe(topic.c_str());
      Serial.print("Unsubscribed from topic: ");
      Serial.println(topic);
    }
  }
}
