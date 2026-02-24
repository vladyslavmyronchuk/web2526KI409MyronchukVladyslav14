#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <AsyncTCP.h>

const char* ssid = "iPhoneMiron";
const char* password = "miron2019";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // Створюємо WebSocket на шляху /ws

// Обробник подій WebSocket
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket клієнт #%u підключився\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket клієнт #%u відключився\n", client->id());
    }
}

void setup() {
    Serial.begin(115200);

    // Ініціалізація LittleFS
    if(!LittleFS.begin(true)){
        Serial.println("Помилка LittleFS!");
        return;
    }

    // Підключення до Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Підключення...");
    }
    Serial.println(WiFi.localIP());

    // Налаштування пінів
    pinMode(2, OUTPUT);

    // Реєстрація WebSocket
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // Маршрути сервера
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });

    // REST-ендпоінт з 2-го завдання
    server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
        int state = digitalRead(2);
        digitalWrite(2, !state);
        String status = (!state) ? "ON" : "OFF";
        String jsonResponse = "{\"status\":\"" + status + "\", \"gpio\":2}";
        request->send(200, "application/json", jsonResponse);
    });

    server.begin();
}

void loop() {
    static unsigned long lastUpdate = 0;
    // Відправка даних на графік кожні 200 мс (5 разів на секунду)
    if (millis() - lastUpdate > 200) {
        int sensorValue = analogRead(34); // Читаємо дані з піна 34
        ws.textAll(String(sensorValue)); // Відправляємо всім клієнтам через WebSocket
        lastUpdate = millis();
    }
    
    ws.cleanupClients(); // Очищення "мертвих" підключень
}