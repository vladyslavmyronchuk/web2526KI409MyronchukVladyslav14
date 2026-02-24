#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <Preferences.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;
Preferences preferences;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) Serial.printf("WS клієнт #%u підключився\n", client->id());
    else if (type == WS_EVT_DISCONNECT) Serial.printf("WS клієнт #%u відключився\n", client->id());
}

void setup() {
    Serial.begin(115200);
    
    // 1. ПОВНЕ ОЧИЩЕННЯ ПЕРЕД СТАРТОМ
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(1000); 

    if(!LittleFS.begin(true)) return;
    pinMode(2, OUTPUT);

    // 2. ЧИТАННЯ ПАРАМЕТРІВ З ПАМ'ЯТІ (NVS)
    preferences.begin("wifi-gate", true); // true - режим тільки для читання
    String ssid = preferences.getString("ssid", "iPhoneMiron"); 
    String pass = preferences.getString("pass", "miron2019");
    preferences.end();

    Serial.print("\n--- СТАРТ ПРИСТРОЮ ---");
    Serial.print("\nСпроба підключення до: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        delay(500);
        Serial.print(".");
    }

    // 3. ПЕРЕВІРКА СТАТУСУ ТА ЗАПУСК AP MODE ЯКЩО ТРЕБА
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[OK] ПІДКЛЮЧЕНО ДО МЕРЕЖІ!");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n[!] WiFi не знайдено. Перехід в AP Mode...");
        WiFi.disconnect();
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        
        if(WiFi.softAP("ESP32-Setup-Miron")) {
            Serial.println("Мережа 'ESP32-Setup-Miron' активна!");
            Serial.print("Адреса налаштування: ");
            Serial.println(WiFi.softAPIP());
        }
        dnsServer.start(DNS_PORT, "*", apIP);
    }

    // 4. НАЛАШТУВАННЯ СЕРВЕРА
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });

    // ОБРОБКА ЗБЕРЕЖЕННЯ (ПРАВИЛЬНА)
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
        String new_ssid = request->arg("ssid");
        String new_pass = request->arg("pass");
        
        // Відкриваємо сховище для запису (false)
        preferences.begin("wifi-gate", false);
        preferences.putString("ssid", new_ssid);
        preferences.putString("pass", new_pass);
        preferences.end(); // ЗАКРИВАЄМО, щоб дані збереглися у флеш

        Serial.printf("Дані збережено: %s / %s\n", new_ssid.c_str(), new_pass.c_str());
        request->send(200, "text/plain", "Saved! ESP32 will reboot and connect to " + new_ssid);
        
        delay(2000);
        ESP.restart(); 
    });

    server.onNotFound([](AsyncWebServerRequest *request){
        request->redirect("/");
    });

    server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
        digitalWrite(2, !digitalRead(2));
        String status = (digitalRead(2)) ? "ON" : "OFF";
        request->send(200, "application/json", "{\"status\":\"" + status + "\"}");
    });

    server.begin();
}

void loop() {
    dnsServer.processNextRequest(); 
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 200) {
        ws.textAll(String(analogRead(34)));
        lastUpdate = millis();
    }
    ws.cleanupClients();
}