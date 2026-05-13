#ifndef SECRETS_H
#define SECRETS_H

// Configuración de WiFi
const char* WIFI_SSID = "TU_WIFI_SSID";
const char* WIFI_PASSWORD = "TU_WIFI_PASSWORD";

// Configuración del Broker MQTT
// Reemplaza con la IP de tu computadora (donde corre Docker)
const char* MQTT_BROKER = "192.168.X.X"; 
const int MQTT_PORT = 1883; // Iniciamos con 1883 (sin TLS) para el Ejercicio 1

// Credenciales MQTT (Las configuraremos en EMQX luego)
const char* MQTT_USER = "alumno_ispc";
const char* MQTT_PASSWORD = "secreto_mqtt_123";

#endif
