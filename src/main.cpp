#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <AsyncTCP.h>

const char* ssid = "iPhoneMiron";
const char* password = "miron2019";

AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);

    // Ініціалізація LittleFS
    if(!LittleFS.begin(true)){
        Serial.println("Помилка LittleFS!");
        return;
    }


    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Підключення...");
    }
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.begin();
}

void loop() {
  
}