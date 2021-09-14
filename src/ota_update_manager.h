/*
์website: ota.neware.dev
*/

#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Update.h>
#include <HTTPClient.h>

class OTA_Manager
{
public:
  OTA_Manager();
  void setup(String version, String hostname, String token);
  void run();
  bool getVersion();
  bool getFileCode(fs::FS &fs);
  void UPDATE(fs::FS &fs);

private:
};
