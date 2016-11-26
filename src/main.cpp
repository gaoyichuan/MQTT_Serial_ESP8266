/**
  MQTTSerialRelay
    Yichuan Gao @2016
    A relay between MQTT protocol and Serial port working on ESP8266 module

  Dependency:
    PubSubClient library

  Usage:
    Arduino to ESP8266:
      Publish: "P${Topic}${Message}"
      Subscribe: "S${Topic}"
    ESP8266 to Arduino:
      Received a message: "R${Topic}${Message}"
      WiFi connected successfully: "SUCCESS"
      Any error: "ERROR${state}"
      Unknown action: "UNKNOWN${action}"

  Default baud rate: 9600
**/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <sstream>
#include <string>
#include <vector>

#define debug

const char *ssid = "";           // WiFi SSID
const char *password = ""; // WiFi PSK

const char *mqtt_broker = "example.com"; // MQTT Broker
const char *mqtt_user = "";         // MQTT Username
const char *mqtt_password = "";        // MQTT Password

WiFiClient espClient;
PubSubClient client(espClient);

std::vector<String> subTopics;
String action, pubTopic, pubMsg, subTopic;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());

#ifdef debug
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif

  Serial.println("SUCCESS");
}

void callback(char *topic, byte *payload, unsigned int length) {
  String recTopic = topic;
  String recMsg = (char *)payload;

#ifdef debug
  Serial.println("Received " + recMsg + " from topic " + recTopic);
#endif

  Serial.println("R$" + recTopic + "$" + recMsg);
}

void reconnect() {
  while (!client.connected()) {
#ifdef debug
    Serial.println("Connection lost, attempting to reconnect...");
#endif
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      for (int i = 0; i < subTopics.size(); i++)
        client.subscribe(subTopics[i].c_str());
    } else {
      Serial.print("ERROR$");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (Serial.available()) {
    while (Serial.available() > 0) {
      action = Serial.readStringUntil('$'); // S: Subscribe, P: Publish
      if (action == "P") {                  // P$Topic$Massage
        Serial.read();
        pubTopic = Serial.readStringUntil('$');
        Serial.read();
        pubMsg = Serial.readString();
#ifdef debug
        Serial.println("Publishing " + pubMsg + "to topic" + pubTopic);
#endif
        client.publish(pubTopic.c_str(), pubMsg.c_str()); // Publish message
      } else if (action == "S") {                         // S$Topic
        subTopic = Serial.readString();
#ifdef debug
        Serial.println("Subscribing to topic " + subTopic);
#endif
        client.subscribe(subTopic.c_str()); // Subcribe the topic
        subTopics.push_back(subTopic);      // Save the topic to vector
      } else {
        Serial.println("UNKNOWN$" + action);
      }
    }
  }
}
