#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <QMC5883LCompass.h> /


const char* ssid = "STRONG_ATRIA_3AFT";
const char* password = "miron2708";

WebSocketsServer webSocket(81);
WebServer server(80);
QMC5883LCompass compass;

void setup() {
    Serial.begin(115200);

    // 1. Ініціалізація LittleFS
    if(!LittleFS.begin(true)){
        Serial.println("LittleFS Error!");
        return;
    }

    Wire.begin(); 
    compass.init();

    // 3. Підключення до Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // 4. Налаштування HTTP-сервера (віддача index.html)
    server.on("/", HTTP_GET, []() {
        File file = LittleFS.open("/index.html", "r");
        if (!file) {
            server.send(404, "text/plain", "File not found");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });

    server.begin();
    webSocket.begin();

    Serial.println("\nSystem Ready. Open IP in browser.");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
}

void loop() {
    server.handleClient();
    webSocket.loop();

    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 100) { 
        lastUpdate = millis();

 
        compass.read();
        int x = compass.getX();
        int y = compass.getY();
        int z = compass.getZ();

        // Вивід у Serial (для контролю)
        Serial.printf("REAL_MAG | X:%d | Y:%d | Z:%d\n", x, y, z);

      
        JsonDocument doc; // Для ArduinoJson 7.x
        doc["x"] = x;
        doc["y"] = y;
        doc["z"] = z;

        String msg;
        serializeJson(doc, msg);
        webSocket.broadcastTXT(msg);
    }
}