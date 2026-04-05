#include "time.h"
#include "webLogic.h"
#include <Preferences.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <AccelStepper.h>


void setup() {
  Serial.begin(115200);

if (!SPIFFS.begin(true)) {
  Serial.println("Error montando SPIFFS");
  return;}

  webInit();
}

void loop() {
  
  webLoop();
}