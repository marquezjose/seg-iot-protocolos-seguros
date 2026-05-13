#include "secrets.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <time.h>

WiFiClient espClient; // Cliente para conexiones sin TLS (Ejercicio 1)
// WiFiClientSecure espClientSecure; // Lo usaremos después en el Ejercicio 2
PubSubClient client(espClient);

unsigned long lastMsgTime = 0;
unsigned long delayTime = 0;

// Configuración de NTP para obtener el timestamp
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800; // GMT-3 (Argentina)
const int daylightOffset_sec = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    String clientId = "ESP32Client-Lab";

    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("conectado");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

float generarDatoAleatorio() {
  // Genera un valor aleatorio entre 20.0 y 30.0
  return 20.0 + (random(0, 100) / 10.0);
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(MQTT_BROKER, MQTT_PORT);

  // Inicializar servidor de hora
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  randomSeed(analogRead(0));

  // Establecer el primer retardo aleatorio (jitter entre 2s y 9s)
  delayTime = random(2000, 9000);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsgTime > delayTime) {
    lastMsgTime = now;

    // Generar nuevo jitter para el próximo envío (2 a 9 segundos)
    delayTime = random(2000, 9000);

    // Obtener Timestamp
    time_t now_ts;
    time(&now_ts);

    // Crear JSON
    JsonDocument doc;
    doc["device"] = "esp32_lab";
    doc["temperature"] = generarDatoAleatorio();
    doc["timestamp"] = now_ts;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    // Publicar
    Serial.print("Publicando mensaje: ");
    Serial.println(jsonBuffer);
    client.publish("sensores/temperatura", jsonBuffer, 0); // QoS 0
  }
}
