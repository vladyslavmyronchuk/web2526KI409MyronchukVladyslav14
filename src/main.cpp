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
    // Використовуємо вбудований світлодіод (GPIO 2)
    pinMode(2, OUTPUT);

    // Створення REST-ендпоінта для перемикання (Пункт 2 методички)
    server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
        int state = digitalRead(2);
        digitalWrite(2, !state); // Інвертуємо стан
        
        // Формуємо JSON відповідь
        String status = (!state) ? "ON" : "OFF";
        String jsonResponse = "{\"status\":\"" + status + "\", \"gpio\":2}";
        
        request->send(200, "application/json", jsonResponse);
    });
}

void loop() {
  
}