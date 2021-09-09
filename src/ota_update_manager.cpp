#include "ota_update_manager.h"

String hostname = "http://34.87.6.251";
String _version = "1";

String filename = "";

OTA_Manager::OTA_Manager()
{
}

void performUpdate(Stream &updateSource, size_t updateSize)
{
    if (Update.begin(updateSize))
    {
        size_t written = Update.writeStream(updateSource);
        if (written == updateSize)
        {
            Serial.println("Written : " + String(written) + " successfully");
        }
        else
        {
            Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
        }
        if (Update.end())
        {
            Serial.println("OTA done!");
            if (Update.isFinished())
            {
                Serial.println("Update successfully completed. Rebooting.");
            }
            else
            {
                Serial.println("Update not finished? Something went wrong!");
            }
        }
        else
        {
            Serial.println("Error Occurred. Error #: " + String(Update.getError()));
        }
    }
    else
    {
        Serial.println("Not enough space to begin OTA");
    }
}

void OTA_Manager::UPDATE(fs::FS &fs)
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS initialisation failed!");
        while (1)
            yield();
    }

    File updateBin = fs.open("/" + filename);
    if (updateBin)
    {
        if (updateBin.isDirectory())
        {
            Serial.println("Error, update.bin is not a file");
            updateBin.close();
            return;
        }

        size_t updateSize = updateBin.size();

        if (updateSize > 0)
        {
            Serial.println("Try to start update");
            performUpdate(updateBin, updateSize);
        }
        else
        {
            Serial.println("Error, file is empty");
        }

        updateBin.close();

        // whe finished remove the binary from sd card to indicate end of the process
        fs.remove("/" + filename);
    }
    else
    {
        Serial.println("Could not load update.bin from sd root");
    }
}

void OTA_Manager::listSPIFFS()
{
    Serial.println(F("\r\nListing SPIFFS files:"));
    static const char line[] PROGMEM = "=================================================";
    Serial.println(FPSTR(line));
    Serial.println(F("  File name                              Size"));
    Serial.println(FPSTR(line));

    fs::File root = SPIFFS.open("/");
    if (!root)
    {
        Serial.println(F("Failed to open directory"));
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(F("Not a directory"));
        return;
    }

    fs::File file = root.openNextFile();
    while (file)
    {

        if (file.isDirectory())
        {
            Serial.print("DIR : ");
            String fileName = file.name();
            Serial.print(fileName);
        }
        else
        {
            String fileName = file.name();
            Serial.print("  " + fileName);
            // File path can be 31 characters maximum in SPIFFS
            int spaces = 33 - fileName.length(); // Tabulate nicely
            if (spaces < 1)
                spaces = 1;
            while (spaces--)
                Serial.print(" ");
            String fileSize = (String)file.size();
            spaces = 10 - fileSize.length(); // Tabulate nicely
            if (spaces < 1)
                spaces = 1;
            while (spaces--)
                Serial.print(" ");
            Serial.println(fileSize + " bytes");
        }
        file = root.openNextFile();
    }
    Serial.println(FPSTR(line));
    Serial.println();
}

bool OTA_Manager::getFileCode()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS initialisation failed!");
        while (1)
            yield();
    }
    if (SPIFFS.exists("/" + filename) == true)
    {
        Serial.println("OTA: Found " + filename);
        return 0;
    }
    Serial.println("OTA: Download... " + filename);
    if ((WiFi.status() == WL_CONNECTED))
    {
        HTTPClient http;
        http.begin(hostname + "/code_bin/" + filename);
        Serial.println("OTA: [HTTP] GET...");
        int httpCode = http.GET();
        if (httpCode > 0)
        {
            fs::File f = SPIFFS.open("/" + filename, "w+");
            if (!f)
            {
                Serial.println("OTA: file open failed");
                return 0;
            }
            if (httpCode == HTTP_CODE_OK)
            {
                int total = http.getSize();
                int len = total;
                uint8_t buff[128] = {0};
                WiFiClient *stream = http.getStreamPtr();
                Serial.println("OTA: loading...");
                while (http.connected() && (len > 0 || len == -1))
                {
                    size_t size = stream->available();
                    if (size)
                    {
                        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                        f.write(buff, c);
                        if (len > 0)
                        {
                            len -= c;
                        }
                    }
                    yield();
                }
                Serial.println();
                Serial.print("OTA: [HTTP] connection closed or file end.\n");
            }
            f.close();
        }
        else
        {
            Serial.printf("OTA: [HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    return 1;
}

bool OTA_Manager::getVersion(String token)
{
    Serial.println();
    Serial.println("OTA: getVersion...");
    if ((WiFi.status() == WL_CONNECTED))
    {
        HTTPClient http;
        http.begin(hostname + "/get_version?token=" + token);
        int httpCode = http.GET();
        if (httpCode > 0)
        {
            String str = http.getString();
            String versionname;
            int s = 0;
            for (int i = 0; i < str.length(); i++)
            {
                if (str[i] == '~')
                    s = 1;
                if (s == 0)
                    versionname += str[i];
                else if (s == 1 && str[i] != '~')
                    filename += str[i];
            }
            Serial.println("version_name: " + versionname);
            Serial.println("filename: " + filename);
            if (versionname == _version)
            {
                Serial.println("OTA: Code OK");
                http.end();
                return 1;
            }
            else
            {
                Serial.println("OTA: Code not current");
                http.end();
                return 0;
            }
        }
        else
        {
            Serial.println("");
            Serial.println("OTA: Error on HTTP request");
            http.end();
            return 0;
        }
    }
    else
    {
        Serial.println("OTA: Error internet connection");
        return 0;
    }
}