#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

const int RELAY_PIN = 4;
const uint8_t RELAY_ON = LOW;   // active-LOW relay module
const uint8_t RELAY_OFF = HIGH;

const char *TOPIC_CMD = "iotnexus/valve/cmd";        // send OPEN or CLOSE
const char *TOPIC_STATE = "iotnexus/valve/state";    // OPEN / CLOSED
const char *TOPIC_STATUS = "iotnexus/device/status"; // online / offline

const unsigned long WIFI_RETRY_MS = 10000;
const unsigned long MQTT_RETRY_MS = 5000;
const unsigned long FAILSAFE_MS = 10000;                // close valve after 10 s offline
const unsigned long MAX_OPEN_MS = 10UL * 60UL * 1000UL; // never stay open > 10 min

WiFiClient net;
PubSubClient mqtt(net);

bool valveOpen = false;
unsigned long openedAt = 0;
unsigned long lastOnline = 0;
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;
bool wasConnected = false;

void publishState()
{
  mqtt.publish(TOPIC_STATE, valveOpen ? "OPEN" : "CLOSED", true);
}

void setValve(bool open, const char *reason)
{
  if (open == valveOpen)
    return;
  valveOpen = open;
  digitalWrite(RELAY_PIN, open ? RELAY_ON : RELAY_OFF);
  if (open)
    openedAt = millis();
  Serial.printf("[VALVE] %s (%s)\n", open ? "OPEN" : "CLOSED", reason);
  if (mqtt.connected())
    publishState();
}

void onMessage(char *topic, byte *payload, unsigned int length)
{
  String cmd;
  for (unsigned int i = 0; i < length; i++)
    cmd += (char)payload[i];
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == "OPEN")
    setValve(true, "mqtt command");
  else if (cmd == "CLOSE")
    setValve(false, "mqtt command");
  else
    Serial.printf("[MQTT] unknown command: %s\n", cmd.c_str());
}

void maintainWifi()
{
  bool connected = WiFi.status() == WL_CONNECTED;

  if (connected && !wasConnected)
    Serial.printf("[WIFI] connected, IP %s, RSSI %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  if (!connected && wasConnected)
    Serial.println("[WIFI] connection lost");
  wasConnected = connected;

  if (connected)
    return;
  if (millis() - lastWifiAttempt < WIFI_RETRY_MS)
    return;
  lastWifiAttempt = millis();
  Serial.println("[WIFI] reconnecting...");
  WiFi.reconnect();
}

void maintainMqtt()
{
  if (WiFi.status() != WL_CONNECTED || mqtt.connected())
    return;
  if (millis() - lastMqttAttempt < MQTT_RETRY_MS)
    return;
  lastMqttAttempt = millis();

  String clientId = "iotnexus-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.printf("[MQTT] connecting to %s as %s\n", MQTT_HOST, clientId.c_str());

  // Last will: broker publishes "offline" if the ESP32 disappears without closing cleanly
  if (mqtt.connect(clientId.c_str(), TOPIC_STATUS, 1, true, "offline"))
  {
    Serial.println("[MQTT] connected");
    mqtt.publish(TOPIC_STATUS, "online", true);
    mqtt.subscribe(TOPIC_CMD);
    publishState();
  }
  else
  {
    Serial.printf("[MQTT] failed, state=%d\n", mqtt.state());
  }
}

void checkFailsafe()
{
  unsigned long now = millis();

  if (mqtt.connected())
    lastOnline = now;
  else if (valveOpen && now - lastOnline > FAILSAFE_MS)
    setValve(false, "failsafe: connection lost");

  if (valveOpen && now - openedAt > MAX_OPEN_MS)
    setValve(false, "failsafe: max open time");
}

void setup()
{
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF); // valve closed until commanded

  Serial.begin(115200);
  delay(200);
  Serial.println("\n[BOOT] iotnexus valve controller");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttempt = millis();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setSocketTimeout(5);

  lastOnline = millis();
}

void loop()
{
  maintainWifi();
  maintainMqtt();
  mqtt.loop();
  checkFailsafe();
}
