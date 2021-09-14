/*
์website: ota.neware.dev
*/

#include "ota_update_manager.h"

String _version = "";
String _hostname = "";
String _token = "";

String filename = "";
String _size = "";

int state_code = 0;

OTA_Manager::OTA_Manager()
{
}

void OTA_Manager::setup(String version, String hostname, String token)
{
    _version = version;
    _token = token;
    _hostname = hostname;
    Serial.println();
    Serial.println("This_version : " + version);
    Serial.println("token : " + token);
    Serial.println("hostname : " + hostname);
    Serial.println();
}

void OTA_Manager::run()
{
    Serial.println("OTA: GET...");
    if (!getVersion())
    {
        if (state_code != 0)
        {
            Serial.println("OTA: DOWNLOAD...");
            if (getFileCode(SPIFFS))
            {
                Serial.println("OTA: UPDATA...");
                UPDATE(SPIFFS);
            }
            else
            {
                if (state_code == 1)
                {
                    Serial.println("OTA: UPDATA...");
                    UPDATE(SPIFFS);
                }
            }
        }
        else
        {
            Serial.println("OTA: Can't Start Download");
        }
    }
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
            Serial.println("Error,  is not a file");
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
        fs.remove("/" + filename);
        ESP.restart();
    }
    else
    {
        Serial.println("Could not load");
    }
}
bool OTA_Manager::getFileCode(fs::FS &fs)
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS initialisation failed!");
        while (1)
            yield();
    }
    if (SPIFFS.exists("/" + filename) == true)
    {
        File file = SPIFFS.open("/" + filename);
        if (String(file.size()) == _size)
        {
            Serial.println("OTA: Found " + filename);
            state_code = 1;
            return 0;
        }
    }
    Serial.println("OTA: Start...Download... " + filename);
    if ((WiFi.status() == WL_CONNECTED))
    {
        HTTPClient http;
        http.begin(_hostname + "/code_bin/" + filename);
        Serial.println("OTA: [HTTP] GET...");
        int httpCode = http.GET();
        if (httpCode > 0)
        {
            fs::File f = SPIFFS.open("/" + filename, FILE_WRITE);
            if (!f)
            {
                if (fs.remove("/" + filename))
                {
                }
                else
                {
                    Serial.println("OTA: file open failed");
                    state_code = 0;
                    return 0;
                }
            }
            if (httpCode == HTTP_CODE_OK)
            {
                int total = http.getSize();
                int len = total;
                uint8_t buff[128] = {0};
                WiFiClient *stream = http.getStreamPtr();
                Serial.print("OTA: loading...");
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
                        Serial.println(len);
                    }
                    yield();
                }
                Serial.println();
                Serial.print("OTA: Download Successful\n");
            }
            f.close();
        }
        else
        {
            Serial.printf("OTA: [HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
        Serial.println("OTA: OK");
    }
    else
    {
        Serial.println("OTA:(erro) Can't Download File");
    }
    return 1;
}

bool OTA_Manager::getVersion()
{
    Serial.println();
    Serial.println("OTA: getVersion...");
    if ((WiFi.status() == WL_CONNECTED))
    {
        HTTPClient http;
        http.begin(_hostname + "/get_version?token=" + _token);
        int httpCode = http.GET();
        if (httpCode > 0)
        {
            String str = http.getString();
            String versionname;
            int s = 0;
            for (int i = 0; i < str.length(); i++)
            {
                if (str[i] == '~')
                {
                    s++;
                }

                if (s == 0)
                    versionname += str[i];
                else if (s == 1 && str[i] != '~')
                    filename += str[i];
                else if (s == 2 && str[i] != '~')
                    _size += str[i];
            }
            Serial.println("OTA: version_name: " + versionname);
            Serial.println("OTA: filename: " + filename + "  size: " + _size);
            Serial.println();
            if (versionname == _version)
            {
                Serial.println("OTA: Code OK");
                http.end();
                state_code = 0;
                return 1;
            }
            else
            {
                Serial.println("OTA: Code not current");
                Serial.println();
                http.end();
                if (versionname != "")
                {
                    state_code = 1;
                }
                else
                {
                    state_code = 0;
                }
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