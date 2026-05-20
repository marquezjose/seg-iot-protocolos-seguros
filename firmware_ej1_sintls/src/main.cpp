#include "secrets.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <time.h>

WiFiClient espClient; // Cliente para conexiones sin TLS (Ejercicio 1)
PubSubClient client(espClient);

unsigned long lastMsgTime = 0;
unsigned long delayTime = 0;
unsigned long ultimoIntentoReconexion = 0; // Control de tiempo para reconexiones no bloqueantes

// Configuración de NTP para obtener el timestamp
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800; // GMT-3 (Argentina)
const int daylightOffset_sec = 0;

void mostrarMetricasRAM(const char* fase) {
  Serial.printf("\n[MÉTRICA-RAM] %s -> Libre actual: %u bytes | Mínima libre registrada: %u bytes | Usada: %u bytes\n", 
                fase, ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize() - ESP.getFreeHeap());
}

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

bool reconnect() {
  Serial.print("Intentando conexión MQTT no bloqueante...");
  String clientId = "ESP32Client-Lab";

  unsigned long inicioConexion = micros();
  if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    unsigned long latenciaConexion = micros() - inicioConexion;
    Serial.printf("\n[MÉTRICA-LATENCIA] Conexión establecida en: %lu µs (%lu ms)\n", latenciaConexion, latenciaConexion / 1000);
    mostrarMetricasRAM("MQTT Conectado");
    return true;
  } else {
    Serial.print("falló, rc=");
    Serial.println(client.state());
    return false;
  }
}

float generarDatoAleatorio() {
  // Genera un valor aleatorio entre 20.0 y 30.0
  return 20.0 + (random(0, 100) / 10.0);
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Darle tiempo a la consola serial para inicializar
  mostrarMetricasRAM("ANTES de WiFi");

  setup_wifi();
  mostrarMetricasRAM("DESPUÉS de WiFi");

  client.setServer(MQTT_BROKER, MQTT_PORT);

  // Inicializar servidor de hora
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  randomSeed(analogRead(0));

  // Establecer el primer retardo aleatorio (jitter entre 2s y 9s)
  delayTime = random(2000, 9000);
}

void loop() {
  if (!client.connected()) {
    unsigned long ahora = millis();
    if (ahora - ultimoIntentoReconexion > 5000) {
      ultimoIntentoReconexion = ahora;
      if (reconnect()) {
        ultimoIntentoReconexion = 0;
      }
    }
  } else {
    client.loop();
  }

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

    // Mostrar payload formateado en Serial Monitor
    Serial.println("--- Payload JSON publicado ---");
    char prettyBuffer[256];
    serializeJsonPretty(doc, prettyBuffer);
    Serial.println(prettyBuffer);
    Serial.println("------------------------------");

    // Publicar y medir latencia
    unsigned long inicioPub = micros();
    bool pubResult = client.publish("sensores/temperatura", jsonBuffer, 0); // QoS 0
    unsigned long latenciaPub = micros() - inicioPub;

    if (pubResult) {
      Serial.printf("[MÉTRICA-LATENCIA] Publicación exitosa completada en: %lu µs\n", latenciaPub);
    } else {
      Serial.println("[ERROR] Falló la publicación MQTT");
    }
    mostrarMetricasRAM("Post-Publicación");
  }
}
