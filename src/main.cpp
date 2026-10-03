#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"

const int RELAY_PIN = 4;
const uint8_t RELAY_ON = LOW; // active-LOW relay module

const unsigned long WIFI_RETRY_MS = 10000;
const unsigned long STATUS_PRINT_MS = 10000;

unsigned long lastWifiAttempt = 0;
unsigned long lastStatusPrint = 0;
bool wasConnected = false;

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

void setup()
{
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_ON); // valve open, as before

  Serial.begin(115200);
  delay(200);
  Serial.println("\n[BOOT] iotnexus valve controller");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttempt = millis();
}

void loop()
{
  maintainWifi();

  if (millis() - lastStatusPrint >= STATUS_PRINT_MS)
  {
    lastStatusPrint = millis();
    Serial.printf("[STATUS] uptime %lu s, WiFi %s\n", millis() / 1000,
                  WiFi.status() == WL_CONNECTED ? "up" : "down");
  }
}
