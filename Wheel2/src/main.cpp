#include "TB6612FNGController.h"
#include <Arduino.h>
#include "ESPHotspot.h"
#define SSID "ESP8266 2W 1"
#define PASSWORD "12345678"

#define PWMA D1
#define PWMB D2
#define AIN1 D3
#define AIN2 D4
#define BIN1 D5
#define BIN2 D6
#define STDBY D7
#define LED_PIN LED_BUILTIN // or any pin you want

Robo::TB6612FNGController MotorController;
#define ValueCount 2

float realBuffer[ValueCount];
Robo::ESPHotspot Hotspot;
#define CHANNEL 5
void setup() {
  memset(realBuffer, 0, sizeof(float) * ValueCount);

  Serial.begin(115200);

  MotorController.init(PWMA, PWMB, AIN1, AIN2, BIN1, BIN2, STDBY);
  Hotspot.start(SSID, PASSWORD, CHANNEL);

  Serial.println("Started Hotspot");
}

void loop() {
  Hotspot.update();

  if (Hotspot.connected()) {
    if (Hotspot.tryReadExact(realBuffer, ValueCount)) {
      Serial.print("x:");
      Serial.print(realBuffer[0]);
      Serial.print(",");
      Serial.print("y:");
      Serial.println(realBuffer[1]);
      MotorController.update(realBuffer[0], realBuffer[1]);
    }
  }
  else {
    MotorController.update(0,1.f);

  }
  yield();
}
