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
const char *mqtt_client_id = "esp01_ariba";

// function prototypes here
void deviceDiscoveryHA_LightSensor();
void deviceDiscoveryHA_Switch1();
void deviceDiscoveryHA_Switch2();
void deviceDiscoveryHA_Switch3();
void deviceDiscoveryHA_Switch4();
void deviceDiscoveryHA_Switch5();
void deviceDiscoveryHA_Switch6();
void deviceDiscoveryHA_Switch7();
void deviceDiscoveryHA_Switch8();
void deviceDiscoveryHA_Siren();

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
      deviceDiscoveryHA_LightSensor(); // home assistant config sent here
      deviceDiscoveryHA_Switch1();
      deviceDiscoveryHA_Switch2();
      deviceDiscoveryHA_Switch3();
      deviceDiscoveryHA_Switch4();
      deviceDiscoveryHA_Switch5();
      deviceDiscoveryHA_Switch6();
      deviceDiscoveryHA_Switch7();
      deviceDiscoveryHA_Switch8();
      deviceDiscoveryHA_Siren();

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

  //  Read data from serial
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

void deviceDiscoveryHA_LightSensor() {
  char topic[128];
  char buffer[512];
  char uid[128];
  JsonDocument doc;
  // Creating topic for light sensor
  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/sensor/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_LS/config");

  // creating payload for Light Sensor
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_LS");
  doc["name"] = "Light Sensor";
  doc["obj_id"] = "mqtt_light_sensor";
  doc["dev_cla"] = "illuminance";
  doc["uniq_id"] = uid;
  doc["stat_t"] = "esp01_ariba/sensors/lightlevel";
  doc["unit_of_meas"] = "lx";
  doc["value_template"] = "{{value_json.lux}}";
  doc["not_payload_available"] = "not_available";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Control Device";
  device["mf"] = "Ariba";
  device["mdl"] = "ESP01";
  device["sw"] = "1.0";
  device["hw"] = "1.0";
  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
}
void deviceDiscoveryHA_Switch1() {
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
  strcat(uid, "_SW1");          // TODO: change this for further switches
  doc["name"] = "switch1";      // TODO: change this for further switches
  doc["obj_id"] = "switch_one"; // TODO: change this for further switches
  doc["uniq_id"] = uid;
  doc["state_topic"] =
      "esp01_ariba/switch1"; // TODO: change this for further switches
  doc["command_topic"] =
      "esp01_ariba/switch1/set"; // TODO: change this for further switches
  subTopic =
      "esp01_ariba/switch1/set"; // TODO: change this for further switches

  doc["value_template"] = "{{value_json.state}}";
  doc["state_template"] = "{{value_json.state}}";
  doc["payload_on"] = "on";
  doc["payload_off"] = "off";
  doc["state_on"] = "on";
  doc["state_off"] = "off";
  doc["optimistic"] = "false";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Control Device";
  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
  client.subscribe(subTopic.c_str());
}
void deviceDiscoveryHA_Switch2() {
  char topic[128];
  char buffer[512];
  char uid[128];
  String subTopic;
  JsonDocument doc;

  // SWITCH 2 HERE
  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/switch/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_sw2/config"); // TODO: change this for further switches

  // creating payload for switch1
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_SW2");          // TODO: change this for further switches
  doc["name"] = "switch2";      // TODO: change this for further switches
  doc["obj_id"] = "switch_two"; // TODO: change this for further switches
  doc["uniq_id"] = uid;
  doc["state_topic"] =
      "esp01_ariba/switch2"; // TODO: change this for further switches
  doc["command_topic"] =
      "esp01_ariba/switch2/set"; // TODO: change this for further switches
  subTopic =
      "esp01_ariba/switch2/set"; // TODO: change this for further switches

  doc["value_template"] = "{{value_json.state}}";
  doc["state_template"] = "{{value_json.state}}";
  doc["payload_on"] = "on";
  doc["payload_off"] = "off";
  doc["state_on"] = "on";
  doc["state_off"] = "off";
  doc["optimistic"] = "false";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Control Device";
  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
  client.subscribe(subTopic.c_str());
}

void deviceDiscoveryHA_Switch3() {
  char topic[128];
  char buffer[512];
  char uid[128];
  String subTopic;
  JsonDocument doc;

  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/switch/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_sw3/config"); // TODO: change this for further switches

  // creating payload for switch1
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_SW3");            // TODO: change this for further switches
  doc["name"] = "switch3";        // TODO: change this for further switches
  doc["obj_id"] = "switch_three"; // TODO: change this for further switches
  doc["uniq_id"] = uid;
  doc["state_topic"] =
      "esp01_ariba/switch3"; // TODO: change this for further switches
  doc["command_topic"] =
      "esp01_ariba/switch3/set"; // TODO: change this for further switches
  subTopic =
      "esp01_ariba/switch3/set"; // TODO: change this for further switches

  doc["value_template"] = "{{value_json.state}}";
  doc["state_template"] = "{{value_json.state}}";
  doc["payload_on"] = "on";
  doc["payload_off"] = "off";
  doc["state_on"] = "on";
  doc["state_off"] = "off";
  doc["optimistic"] = "false";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Control Device";
  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
  client.subscribe(subTopic.c_str());
}

void deviceDiscoveryHA_Switch4() {
  char topic[128];
  char buffer[512];
  char uid[128];
  String subTopic;
  JsonDocument doc;
  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/switch/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_sw4/config"); // TODO: change this for further switches

  // creating payload for switch1
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_SW4");           // TODO: change this for further switches
  doc["name"] = "switch4";       // TODO: change this for further switches
  doc["obj_id"] = "switch_four"; // TODO: change this for further switches
  doc["uniq_id"] = uid;
  doc["state_topic"] =
      "esp01_ariba/switch4"; // TODO: change this for further switches
  doc["command_topic"] =
      "esp01_ariba/switch4/set"; // TODO: change this for further switches
  subTopic =
      "esp01_ariba/switch4/set"; // TODO: change this for further switches

  doc["value_template"] = "{{value_json.state}}";
  doc["state_template"] = "{{value_json.state}}";
  doc["payload_on"] = "on";
  doc["payload_off"] = "off";
  doc["state_on"] = "on";
  doc["state_off"] = "off";
  doc["optimistic"] = "false";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Control Device";
  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
  client.subscribe(subTopic.c_str());
}

void deviceDiscoveryHA_Switch5() {
  char topic[128];
  char buffer[512];
  char uid[128];
  String subTopic;
  JsonDocument doc;
  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/switch/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_sw5/config"); // TODO: change this for further switches

  // creating payload for switch1
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_SW5");           // TODO: change this for further switches
  doc["name"] = "switch5";       // TODO: change this for further switches
  doc["obj_id"] = "switch_five"; // TODO: change this for further switches
  doc["uniq_id"] = uid;
  doc["state_topic"] =
      "esp01_ariba/switch5"; // TODO: change this for further switches
  doc["command_topic"] =
      "esp01_ariba/switch5/set"; // TODO: change this for further switches
  subTopic =
      "esp01_ariba/switch5/set"; // TODO: change this for further switches

  doc["value_template"] = "{{value_json.state}}";
  doc["state_template"] = "{{value_json.state}}";
  doc["payload_on"] = "on";
  doc["payload_off"] = "off";
  doc["state_on"] = "on";
  doc["state_off"] = "off";
  doc["optimistic"] = "false";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Control Device";
  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
  client.subscribe(subTopic.c_str());
}

void deviceDiscoveryHA_Switch6() {
  char topic[128];
  char buffer[512];
  char uid[128];
  String subTopic;
  JsonDocument doc;

  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/switch/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_sw6/config"); // TODO: change this for further switches

  // creating payload for switch1
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_SW6");          // TODO: change this for further switches
  doc["name"] = "switch6";      // TODO: change this for further switches
  doc["obj_id"] = "switch_six"; // TODO: change this for further switches
  doc["uniq_id"] = uid;
  doc["state_topic"] =
      "esp01_ariba/switch6"; // TODO: change this for further switches
  doc["command_topic"] =
      "esp01_ariba/switch6/set"; // TODO: change this for further switches
  subTopic =
      "esp01_ariba/switch6/set"; // TODO: change this for further switches

  doc["value_template"] = "{{value_json.state}}";
  doc["state_template"] = "{{value_json.state}}";
  doc["payload_on"] = "on";
  doc["payload_off"] = "off";
  doc["state_on"] = "on";
  doc["state_off"] = "off";
  doc["optimistic"] = "false";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Control Device";
  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
  client.subscribe(subTopic.c_str());
}
void deviceDiscoveryHA_Switch7() {
  char topic[128];
  char buffer[512];
  char uid[128];
  String subTopic;
  JsonDocument doc;

  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/switch/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_sw7/config"); // TODO: change this for further switches

  // creating payload for switch1
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_SW7");            // TODO: change this for further switches
  doc["name"] = "switch7";        // TODO: change this for further switches
  doc["obj_id"] = "switch_seven"; // TODO: change this for further switches
  doc["uniq_id"] = uid;
  doc["state_topic"] =
      "esp01_ariba/switch7"; // TODO: change this for further switches
  doc["command_topic"] =
      "esp01_ariba/switch7/set"; // TODO: change this for further switches
  subTopic =
      "esp01_ariba/switch7/set"; // TODO: change this for further switches

  doc["value_template"] = "{{value_json.state}}";
  doc["state_template"] = "{{value_json.state}}";
  doc["payload_on"] = "on";
  doc["payload_off"] = "off";
  doc["state_on"] = "on";
  doc["state_off"] = "off";
  doc["optimistic"] = "false";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Control Device";

  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
  client.subscribe(subTopic.c_str());
}

void deviceDiscoveryHA_Switch8() {
  char topic[128];
  char buffer[512];
  char uid[128];
  String subTopic;
  JsonDocument doc;

  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/switch/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_sw8/config"); // TODO: change this for further switches

  // creating payload for switch1
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_SW8");            // TODO: change this for further switches
  doc["name"] = "switch8";        // TODO: change this for further switches
  doc["obj_id"] = "switch_eight"; // TODO: change this for further switches
  doc["uniq_id"] = uid;
  doc["state_topic"] =
      "esp01_ariba/switch8"; // TODO: change this for further switches
  doc["command_topic"] =
      "esp01_ariba/switch8/set"; // TODO: change this for further switches
  subTopic =
      "esp01_ariba/switch8/set"; // TODO: change this for further switches

  doc["value_template"] = "{{value_json.state}}";
  doc["state_template"] = "{{value_json.state}}";
  doc["payload_on"] = "on";
  doc["payload_off"] = "off";
  doc["state_on"] = "on";
  doc["state_off"] = "off";
  doc["optimistic"] = "false";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Control Device";

  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
  client.subscribe(subTopic.c_str());
}

void deviceDiscoveryHA_Siren() {
  char topic[128];
  char buffer[512];
  char uid[128];
  String subTopic;
  JsonDocument doc;

  doc.clear();
  // creating topic here
  strcpy(topic, "homeassistant/siren/");
  strcat(topic, mqtt_client_id);
  strcat(topic, "_sr/config");

  // creating payload for switch1
  strcpy(uid, mqtt_client_id);
  strcat(uid, "_SR");      // TODO: change this for further switches
  doc["name"] = "siren";   // TODO: change this for further switches
  doc["obj_id"] = "siren"; // TODO: change this for further switches
  doc["uniq_id"] = uid;
  doc["state_topic"] =
      "esp01_ariba/siren"; // TODO: change this for further switches
  doc["command_topic"] =
      "esp01_ariba/siren/set";        // TODO: change this for further switches
  subTopic = "esp01_ariba/siren/set"; // TODO: change this for further switches

  doc["value_template"] = "{{value_json.state}}";
  // doc["state_template"] = "{{value_json.state}}";
  // doc["command_template"] = "{{value_json.state}}";
  doc["payload_on"] = "on";
  doc["payload_off"] = "off";
  doc["state_on"] = "on";
  doc["state_off"] = "off";
  doc["optimistic"] = "false";

  JsonObject device = doc.createNestedObject("device");
  device["ids"] = mqtt_client_id;
  device["name"] = "Control Device";

  serializeJson(doc, buffer);
  // Publish discovery topic and payload (with retained flag)
  client.publish(topic, buffer, true);
  client.subscribe(subTopic.c_str());
}