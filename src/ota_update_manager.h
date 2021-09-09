#include <Arduino.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <FS.h>
#elif defined (ESP32)
  #include <WiFi.h>
  #include <FS.h>
  #include <SPIFFS.h>
  #include <Update.h>
  #include <HTTPClient.h>
#endif

class OTA_Manager
{
    public:
        OTA_Manager();
        bool getVersion(String token);
        bool getFileCode();
        void listSPIFFS();
        void UPDATE(fs::FS &fs);
    private:

};
