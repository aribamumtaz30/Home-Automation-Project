#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#define readyPin 2
// #define WDT_TIMEOUT 20000

// WiFi credentials
const char *ssid = "";    // !! Enter WiFi name here !!
const char *password = ""; // !! Enter WiFi password here !!

// MQTT Broker settings
const char *mqtt_server = "emqx.home.mwaleedh.com.pk";
const int mqtt_port = 8883;
const char *mqtt_user = ""; // !! Provided server username here !!
const char *mqtt_password = ""; //!! Provided server password here !!
const char *mqtt_client_id = "esp01_rameeza";

// function prototypes here
void deviceDiscoveryHA_BellButton();
void deviceDiscoveryHA_DoorSensor();
void deviceDiscoveryHA_DoorSwitch();

void reconnect();
void nonBlockingDelay(unsigned long interval);

// pre defined objects here

WiFiClientSecure espClient;
PubSubClient client(espClient);

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.flush();
  Serial.print("topic:");
  Serial.print(topic);
  // String recv_payload = String((char *)payload);
  Serial.print("|payload:");
  char *recv_payload = (char *)malloc(length);
  for (int i = 0; i < length; i++) {
    recv_payload[i] = (char)payload[i];
    Serial.print((char)payload[i]);
  }
  Serial.print("\n");
  // strcat(recv_payload,"@");
  // Serial.print(recv_payload);
  // Serial.print("\n")
  // Serial.println(length);

  // Parse JSON payload
  // JsonDocument doc;
  // DeserializationError error = deserializeJson(doc, payload, length);
  // if (error) {
  //   String recv_payload = String((char *)payload);
  //   Serial.print("|payload:");
  //   Serial.println(recv_payload);
  // }
  // digitalWrite(LED, LOW);

  // Extract values from JSON and update variables
  // switchState = doc["switch"];
  // temperature = doc["temperature"];

  // Serial.print("Switch state: ");
  // Serial.println(switchState);
  // Serial.print("Temperature: ");
  // Serial.println(temperature);
  delay(1000);
}

unsigned long previousMillis = 0;

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
  client.setBufferSize(512); // increasing buffer size

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("MQTT connected");
      deviceDiscoveryHA_DoorSensor();
      deviceDiscoveryHA_DoorSwitch();
      deviceDiscoveryHA_BellButton();

      digitalWrite(
          readyPin,
          HIGH); // GPIO2 goes high when server connection is established

    } else {
      digitalWrite(readyPin, LOW); // GPIO2 goes low when server is disconnected
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retry in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  // ESP.wdtFeed();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

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
        if (!error) { // error checking json format
          client.publish(topic.c_str(), payload.c_str());

          // uncomment this for debugging

          // Serial.println("Published message:");
          // Serial.println(payload);
        }
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

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("MQTT connected");
      digitalWrite(
          readyPin,
          HIGH); // GPIO2 goes high when server connection is established
    } else {
      digitalWrite(readyPin, LOW); // GPIO2 goes low when server is disconnected
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retry in 5 seconds");
      delay(5000);
    }
  }
}

void nonBlockingDelay(unsigned long interval) {
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    // Perform action here...
  }
}

void deviceDiscoveryHA_DoorSensor() {
  char topic[128];
  char buffer[512];
  char uid[128];
  JsonDocument doc;

  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/binary_sensor/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_BS/config");

  // creating payload for Window Sensor
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_BS");
  doc["name"] = "Dooor Sensor";
  doc["obj_id"] = "mqtt_door_sensor";
  doc["uniq_id"] = uid;
  doc["stat_t"] = "esp01_rameeza/sensors/door_sensor";
  doc["value_template"] = "{{value_json.state}}";
  doc["payload_on"] = "open";
  doc["payload_off"] = "close";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Door Bell";
  device["mf"] = "Rameeza";
  device["mdl"] = "ESP01";
  device["sw"] = "1.0";
  device["hw"] = "1.0";
  // device["cu"] = "http://192.168.1.226/config";  //web interface for device,
  // with discovery toggle
  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
}

void deviceDiscoveryHA_DoorSwitch() {
  char topic[128];
  char buffer[512];
  char uid[128];
  String subTopic;
  JsonDocument doc;

  // SWITCH 1 HERE
  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/switch/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_sw1/config"); // TODO: change this for further switches

  // creating payload for switch1
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_DS");            // TODO: change this for further switches
  doc["name"] = "Door Switch";   // TODO: change this for further switches
  doc["obj_id"] = "door_switch"; // TODO: change this for further switches
  doc["uniq_id"] = uid;
  doc["state_topic"] =
      "esp01_rameeza/door_switch"; // TODO: change this for further switches
  doc["command_topic"] =
      "esp01_rameeza/door_switch/set"; // TODO: change this for further switches
  subTopic =
      "esp01_rameeza/door_switch/set"; // TODO: change this for further switches

  doc["value_template"] = "{{value_json.state}}";
  doc["state_template"] = "{{value_json.state}}";
  doc["payload_on"] = "on";
  doc["payload_off"] = "off";
  doc["state_on"] = "on";
  doc["state_off"] = "off";
  doc["optimistic"] = "false";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Door Bell";
  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
  client.subscribe(subTopic.c_str());
}

void deviceDiscoveryHA_BellButton() {

  char topic[128];
  char buffer[512];
  char uid[128];
  JsonDocument doc;

  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/binary_sensor/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_BTN/config");

  // creating payload for Window Sensor
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_BTN");
  doc["name"] = "Bell Button";
  doc["obj_id"] = "mqtt_bell_button";
  doc["uniq_id"] = uid;
  doc["stat_t"] = "esp01_rameeza/buttons/bell_button/state";
  doc["value_template"] = "{{value_json.state}}";
  doc["payload_on"] = "pressed";
  doc["payload_off"] = "de_pressed";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Door Bell";
  // device["cu"] = "http://192.168.1.226/config";  //web interface for device,
  // with discovery toggle
  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
}

//   char topic[128];
//   char buffer[512];
//   char uid[128];
//   JsonDocument doc;

//   doc.clear();
//   // creating topic here
//   strcpy(topic, "homeassistant/binary_sensor/");
//   strcat(topic, mqtt_client_id);
//   strcat(topic, "_BTN/config");

//   // creating payload for Window Sensor
//   strcpy(uid, mqtt_client_id);
//   strcat(uid, "_BTN");
//   doc["name"] = "Bell Button";
//   doc["obj_id"] = "bell_button";
//   doc["uniq_id"] = uid;

//   doc["value_template"] = "{{value_json.event}}";
//   doc["event_types"] = "press";

//   doc["state_topic"] = "esp01_rameeza/buttons/bell_button/state";
//   // doc["automation_type"] = "trigger";
//   // doc["payload"] = "pressed";
//   // doc["type"] = "button_short_press";
//   // doc["subtype"] = "button_1_short_press";

//   JsonObject device = doc.createNestedObject("device");
//   device["ids"] = mqtt_client_id;
//   device["name"] = "Door Bell";
//   serializeJson(doc, buffer);
//   // Publish discovery topic and payload (with retained flag)
//   client.publish(topic, buffer, true);
// }

// void deviceDiscoveryHA_DoorSensor() {