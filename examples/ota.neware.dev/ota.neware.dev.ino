/*
 * Websit: ota.neware.dev
 */
#include <ota_update_manager.h>

const char * ssid = "";
const char * password = "";

OTA_Manager ota;

String this_version = "";
String token = "";
String hostname = "";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  ota.setup(this_version, hostname, token);

  ota.run();

  Serial.println();
  Serial.println("Program 1");
}

void loop() {


}