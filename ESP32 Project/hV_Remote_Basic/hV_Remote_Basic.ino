///
/// @file hV_Remote_Basic.ino
/// @brief Remote e-Paper Pictures Panel
///
/// @details Example for PDLS
///
/// @author Rei Vilo
/// @date 14 Feb 2022
/// @version 102
///
/// @copyright (c) Rei Vilo, 2010-2022
/// @copyright CC = BY SA NC
///
/// @see ReadMe.txt for references
/// @n
///

// SDK
#include "Arduino.h"

// Configuration

// Set parameters
#define myLED 2

// Include application, user and local libraries
#include "Credentials.h"
#include <WiFi.h>
#include <PubSubClient.h>

// Define structures and classes

// Define variables and constants

// === Pervasive Displays iTC
// --- Small
#include "PDLS_EXT3_Basic_Fast.h"
Screen_EPD_EXT3_Fast myScreen(eScreen_EPD_EXT3_271_Fast, boardESP32DevKitC);

// Variables
WiFiClient myWiFi;
PubSubClient myMQTT(myWiFi);
bool flagActive = true;

// Prototypes

// Utilities

// Functions
void callbackMQTT(char * topic, byte * message, unsigned int length)
{
    bool flagFlush = false;
    Serial.print("MQTT Topic= '");
    Serial.print(topic);
    Serial.println("'");

    // Topic/sub-topic
    if (String(topic) == "image/show")
    {
        if ((message[0] != 'P') or (message[1] != '4'))
        {
            Serial.print("Header ");
            Serial.print((char)message[0]);
            Serial.print(".");
            Serial.print((char)message[1]);
                  Serial.println("Error - Not P4");

            return;
        }

        uint8_t * c;
        c = message + 3;

        uint16_t minPartialH = 0;
        uint16_t minPartialV = 0;
        uint16_t maxPartialH = 0;
        uint16_t maxPartialV = 0;

        while (isdigit(*c))
        {
            maxPartialH *= 10;
            maxPartialH += (*c - '0');
            c++;
        }
        c++; // 0x20
        while (isdigit(*c))
        {
            maxPartialV *= 10;
            maxPartialV += (*c - '0');
            c++;
        }
        c++; // 0x0a

        myScreen.setOrientation(0); // native orientation
        for (uint16_t y = minPartialV; y < maxPartialV; y++)
        {
            for (uint16_t x = minPartialH; x < maxPartialH; x += 8)
            {
                for (uint16_t z = 0; z < 8; z++)
                {
                    if (bitRead(*c, 7 - z) > 0)
                    {
                        myScreen.point(x + z, y, myColours.black);
                    }
                    else
                    {
                        myScreen.point(x + z, y, myColours.white);
                    }
                }
                c++; // next
            }
        }

        flagFlush = true;
    }
    else if (String(topic) == "image/clear")
    {
        myScreen.clear();
        flagFlush = true;
    }
    else if (String(topic) == "image/led")
    {
        Serial.print(". Turn LED ");
        if ((message[0] == 'o') and (message[1] == 'n'))
        {
            Serial.println("on");
            digitalWrite(myLED, HIGH);
        }
        else if ((message[0] == 'o') and (message[1] == 'f') and (message[2] == 'f'))
        {
            Serial.println("on");
            digitalWrite(myLED, LOW);
        }
        else
        {
            Serial.println("?");
        }
    }
    else if (String(topic) == "image/end")
    {
        if ((message[0] == 'e') and (message[1] == 'n') and (message[2] == 'd'))
        {
            Serial.print(". End ");
            flagActive = false;
        }
    }

    if (flagFlush == true)
    {
        myScreen.flush();
    }
}

void setupWiFi()
{
    delay(10);

    WiFi.begin(mySSID, myPassword);

    Serial.print("SSID...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.print(" ");
    Serial.println(mySSID);

    Serial.print("IP... ");
    Serial.println(WiFi.localIP());
}

void connectMQTT()
{
    // Loop until we're reconnected
    while (!myMQTT.connected())
    {
        Serial.print("MQTT... ");
        // Attempt to connect
        if (myMQTT.connect("ESP32 Remote"))
        {
            Serial.println("done");
            // Subscribe
            myMQTT.subscribe("image/#");
        }
        else
        {
            Serial.print("! ");
            Serial.print(myMQTT.state());
            Serial.println(". Wait...");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

// Add setup code
void setup()
{
    pinMode(myLED, OUTPUT);

    Serial.begin(115200);
    delay(500);
    // wait(4);

    Serial.println("");
    Serial.println("");
    Serial.println("=== " __FILE__);
    Serial.println("=== " __DATE__ " " __TIME__);
    Serial.println("");

    // Screen
    myScreen.begin();
    myScreen.clear();
    myScreen.flush();

    // Default orientation
    myScreen.setOrientation(0);

    // WiFi
    setupWiFi();

    // MQTT
    myMQTT.setBufferSize(6144);
    myMQTT.setServer(myBrokerIP, 1883);
    myMQTT.setCallback(callbackMQTT);
}

// Add loop code
void loop()
{
    while (flagActive)
    {
        if (!myMQTT.connected())
        {
            connectMQTT();
        }
        myMQTT.loop();
    }

    // End
    myScreen.clear();
    myScreen.flush();

    Serial.println("=== End");
    while (true);
}
