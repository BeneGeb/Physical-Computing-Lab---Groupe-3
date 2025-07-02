#include <Arduino.h>
#include <WiFi.h>
#include <Audio.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>

#include "secrets.h"
#include "pinconfig.h"
#include "prompts.h"
#include "database.h"

// --- Light sensor setup ---
#define LIGHT_THRESHOLD 3500
int sensor_value = 0;

// --- Audio setup ---
Audio *audio;

// --- Camera setup ---
const uint8_t END_MARKER[] = {0xFF, 0xD9, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF};
const size_t END_MARKER_LEN = sizeof(END_MARKER);
const int MAX_IMAGE_SIZE = 100 * 256;
uint8_t imageBuffer[MAX_IMAGE_SIZE];
size_t imageIndex = 0;

unsigned long startTimeTimeout = 0;
const unsigned long audioTimeout = 30000; // 30 seconds timeout for audio playback
const unsigned long imageTimeout = 5000;  // 5 seconds timeout for image capture

const bool DEBUG_MODE = true;

void debugPrint(String message)
{
    if (DEBUG_MODE)
    {
        Serial.print(message);
    }
}

void debugPrintln(String message)
{
    if (DEBUG_MODE)
    {
        Serial.println(message);
    }
}

bool isLightOn()
{
    sensor_value = analogRead(PIN_LIGHT_SENSOR);
    // debugPrintln("Current light sensor value: " + String(sensor_value));
    return sensor_value < LIGHT_THRESHOLD;
}

void printRawImageData()
{
    Serial.println("Raw image data:");
    for (size_t i = 0; i < imageIndex; i++)
    {
        if (imageBuffer[i] < 16)
        {
            Serial.print("0");
        }
        Serial.print(String(imageBuffer[i], HEX) + " ");
    }
    Serial.println("");
}

String urlEncode(const String &str)
{
    String encoded = "";
    char c;
    char code0;
    char code1;
    for (int i = 0; i < str.length(); i++)
    {
        c = str.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == ' ')
        {
            encoded += c;
        }
        else
        {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9)
                code1 = (c & 0xf) - 10 + 'A';
            code0 = ((c >> 4) & 0xf) + '0';
            if (((c >> 4) & 0xf) > 9)
                code0 = ((c >> 4) & 0xf) - 10 + 'A';
            encoded += '%';
            encoded += code0;
            encoded += code1;
        }
    }
    return encoded;
}

void playTTSAudio(const String &input)
{
    String result = urlEncode(input);
    result.replace(" ", "+");
    String finalUrl = String(TTS_URL) + "text=" + result;

    audio = new Audio();
    audio->setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio->setVolume(10);
    audio->connecttohost(finalUrl.c_str());
    startTimeTimeout = millis();
    while (1)
    {
        audio->loop();
        if (millis() - startTimeTimeout > audioTimeout)
        {
            debugPrintln("Audio playback timeout reached.");
            break;
        }
        if (!audio->isRunning())
        {
            debugPrintln("Audio playback finished.");
            break;
        }
    }
    audio->stopSong();
    delete audio;
}

String getAPIResponse(String inputText, bool sendImage = false)
{
    String outputText = "Fehler bei der Kommunikation mit der API";
    String apiUrl = "https://api.openai.com/v1/chat/completions";
    String payload;

    DynamicJsonDocument doc(MAX_IMAGE_SIZE + 512);
    doc["model"] = "gpt-4o";
    JsonArray messages = doc.createNestedArray("messages");
    JsonObject userMessage = messages.createNestedObject();
    userMessage["role"] = "user";

    if (sendImage)
    {
        JsonArray contentArray = userMessage.createNestedArray("content");

        JsonObject textObj = contentArray.createNestedObject();
        textObj["type"] = "text";
        textObj["text"] = inputText;

        JsonObject imageObj = contentArray.createNestedObject();
        imageObj["type"] = "image_url";
        imageObj["image_url"]["url"] = "data:image/jpeg;base64," + base64::encode(imageBuffer, imageIndex - END_MARKER_LEN);
    }
    else
    {
        userMessage["content"] = inputText;
    }
    serializeJson(doc, payload);

    HTTPClient http;
    http.begin(apiUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(CHATGPT_API_KEY));
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode == 200)
    {
        String response = http.getString();
        DynamicJsonDocument jsonDoc(1024);
        deserializeJson(jsonDoc, response);
        outputText = jsonDoc["choices"][0]["message"]["content"].as<String>();
    }
    else
    {
        Serial.printf("Error %i \n", httpResponseCode);
    }
    http.end();
    return outputText;
}

bool imageBufferEndsWithEndMarker()
{
    if (imageIndex < END_MARKER_LEN)
        return false;
    for (size_t i = 0; i < END_MARKER_LEN; ++i)
    {
        if (imageBuffer[imageIndex - END_MARKER_LEN + i] != END_MARKER[i])
        {
            return false;
        }
    }
    return true;
}

bool takeImage()
{
    Serial2.println("capture");
    memset(imageBuffer, 0, MAX_IMAGE_SIZE);
    imageIndex = 0;
    unsigned long start = millis();
    unsigned long timeDifference = 0;
    while (!imageBufferEndsWithEndMarker())
    {
        timeDifference = millis() - start;
        if (timeDifference > imageTimeout)
        {
            Serial.println("Image Capture Error: Timeout reached");
            return false;
        }
        if (imageIndex >= MAX_IMAGE_SIZE)
        {
            Serial.println("Image Capture Error: Buffer overflow");
            return false;
        }

        if (Serial2.available() > 0)
        {
            imageBuffer[imageIndex++] = Serial2.read();
        }
    }
    return true;
}

void setup()
{
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, CAMERA_RX_PIN, CAMERA_TX_PIN);

    // - WiFi setup -
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);

    String ssid = WIFI_SSID;
    String password = WIFI_PASSWORD;
    WiFi.begin(ssid.c_str(), password.c_str());

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    // WiFi connected, print IP to serial monitor
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println(WiFi.localIP());
    Serial.println("");

    // - Light sensor setup -
    pinMode(PIN_LIGHT_SENSOR, OUTPUT);
    digitalWrite(PIN_LIGHT_SENSOR, LOW);

    // - Button setup -
    pinMode(BUTTON_PIN_INSERT, INPUT_PULLUP);
    pinMode(BUTTON_PIN_REMOVE, INPUT_PULLUP);

    // - LED setup -
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);

    // - Database setup -
    initDatabase();
}

void loop()
{
    if (isLightOn())
    {
        digitalWrite(RED_LED_PIN, HIGH);

        // Check if any button was pressed
        if (digitalRead(BUTTON_PIN_INSERT) == LOW || digitalRead(BUTTON_PIN_REMOVE) == LOW)
        {
            bool insertAction = digitalRead(BUTTON_PIN_INSERT) == LOW;
            bool removeAction = digitalRead(BUTTON_PIN_REMOVE) == LOW;

            // Check that only one button was pressed
            if (insertAction != removeAction)
            {
                digitalWrite(YELLOW_LED_PIN, HIGH);
                if (takeImage())
                {
                    debugPrintln("Image captured successfully. Image size: " + String(imageIndex) + " bytes");
                    // printRawImageData();
                    bool hasBarcode = false;
                    String productName = "";

                    // TODO: Check image for Barcode
                    // Das hier geht prinzipiell, aber wird ja denke ich nicht die Lösung am Ende. Barcodes richtig einlesen konnte ChatGPT bei mir nicht.
                    // productName = getAPIResponse("Gebe mir in einem Wort zurück, ob sich auf dem Bild ein Barcode befindet oder nicht.", true);
                    // debugPrintln("Is there a Barcode? Recognized by ChatGPT: " + productName);

                    if (hasBarcode)
                    {
                        // TODO: Handle Barcode
                    }
                    else
                    {
                        String finalPrompt = String(PROMPT_IMAGE_RECOGNITION) + "\n\n" + getDatabaseContentForPrompt();
                        productName = getAPIResponse(finalPrompt, true);

                        debugPrintln("Product name recognized by ChatGPT: " + productName);
                    }

                    if (insertAction)
                    {
                        addItemToDatabase(productName.c_str());
                        String ttsResponse = getAPIResponse(PROMPT_ADD_PRODUCT + productName);
                        debugPrintln("TTS Response (add product): " + ttsResponse);
                        playTTSAudio(ttsResponse);
                    }
                    if (removeAction)
                    {
                        removeItemFromDatabase(productName.c_str());
                        String ttsResponse = getAPIResponse(PROMPT_REMOVE_PRODUCT + productName);
                        debugPrintln("TTS Response (remove product): " + ttsResponse);
                        playTTSAudio(ttsResponse);
                    }
                    printDatabase();
                }
                digitalWrite(YELLOW_LED_PIN, LOW);
            }
        }
    }
    else
    {
        digitalWrite(RED_LED_PIN, LOW);
        delay(500);
    }
}