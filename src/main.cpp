#include <Arduino.h>

const int RELAY_PIN = 4;

void setup()
{
  pinMode(RELAY_PIN, OUTPUT);

  // Active-LOW relay:
  // LOW = relay ON = solenoid powered = valve OPEN
  digitalWrite(RELAY_PIN, LOW);
}

void loop()
{
  // Keep valve open continuously
}