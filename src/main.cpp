#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <Preferences.h>

// Дані для автентифікації (Етап 5)
const char* www_username = "admin";
const char* www_password = "miron2026"; // Твій пароль

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
    
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(1000); 

    if(!LittleFS.begin(true)) return;
    pinMode(2, OUTPUT);

    preferences.begin("wifi-gate", true);
    String ssid = preferences.getString("ssid", "iPhoneMiron"); 
    String pass = preferences.getString("pass", "miron2019");
    preferences.end();

    Serial.print("\n--- СТАРТ ПРИСТРОЮ ---");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[OK] ПІДКЛЮЧЕНО ДО МЕРЕЖІ: " + WiFi.localIP().toString());
    } else {
        Serial.println("\n[!] Запуск Точки Доступу...");
        WiFi.disconnect();
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP("ESP32-Setup-Miron");
        dnsServer.start(DNS_PORT, "*", apIP);
    }

    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // Впровадження Basic Auth (Етап 5)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate(www_username, www_password))
            return request->requestAuthentication();
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
        if(!request->authenticate(www_username, www_password))
            return request->requestAuthentication();
            
        String new_ssid = request->arg("ssid");
        String new_pass = request->arg("pass");
        
        preferences.begin("wifi-gate", false);
        preferences.putString("ssid", new_ssid);
        preferences.putString("pass", new_pass);
        preferences.end();

        request->send(200, "text/plain", "Saved! Rebooting...");
        delay(2000);
        ESP.restart(); 
    });

    server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate(www_username, www_password))
            return request->requestAuthentication();
            
        digitalWrite(2, !digitalRead(2));
        String status = (digitalRead(2)) ? "ON" : "OFF";
        request->send(200, "application/json", "{\"status\":\"" + status + "\"}");
    });

    server.onNotFound([](AsyncWebServerRequest *request){
        request->redirect("/");
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