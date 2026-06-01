#ifndef SECRETS_H
#define SECRETS_H

// Configuración de WiFi
const char *WIFI_SSID = "Claro.400";
const char *WIFI_PASSWORD = "T4sm4nia400";

// Configuración del Broker MQTT
// Reemplaza con la IP de tu computadora (donde corre Docker)
const char *MQTT_BROKER = "192.168.100.19";
const int MQTT_PORT = 8883; // Puerto 8883 para MQTT sobre TLS (MQTTS)

// Credenciales MQTT (Las configuraremos en EMQX luego)
const char *MQTT_USER = "alumno_ispc";
const char *MQTT_PASSWORD = "secreto_mqtt_123";

const char *ca_cert = R"EOF(-----BEGIN CERTIFICATE-----
MIICmDCCAYACAQEwDQYJKoZIhvcNAQELBQAwEjEQMA4GA1UEAwwHSVNQQ19DQTAe
Fw0yNjA1MTUwMDAwMDBaFw0yNzA1MTUwMDAwMDBaMBIxEDAOBgNVBAMMB0lTUENf
Q0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDEIcY7Uxac3i69y8Iv
b5SHdPoCaHeEWvnmN/Jw42GK7fg84PwWprj3hBkI0tIKymu6Kevckr1yyAVghkoZ
VFYlPcwunGLecAHkYc27GoJKsluzf0OeCyEcqB8FLmvTR3nDuPwHb6/nTMgK1aD1
Jo9H4Lf6gDJCeZSLeVFfJD0oX4IGONQp5/lNdHfN/rMMgh5Uws6PmiMUX2Y93ZNl
NPEBdRzC+62fota+gKXyJx+J/KwwXBZ2sgTvzgzuYEJrL4k8A83skfxbwlJumza0
9quQTDZUZjnLAawcGGqvEJfJB6FACVUWqL1x2zZaCwhW5i7riIOp55qri0cCGYiJ
XhKlAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAGtSzIxhPM6ILstQGB9/BP2YBV3Y
77BCpmo+ej0lfJWBXkQLGVDN3lOZcE/247Id2bka8LMPJea5GlQmJaIl7SeRBmu9
48V/+YbxJ5zqLXcNpJmlrhLZ8dDc4sFTWYKxR0kyOrjgOm65BgmouRXw6Ite/K7h
PAr6T3/M08S3sUOQf2mmYJcpA/fumMFrn0r1kOQUpzacGsoyJ2+RGfI1K/eUnJ/d
XNvDXWokLw+buO01PS2C5tOn8zL6rboTJpG5wUV50trl0dg6+gv1kGA/cDImhl3a
t4ogxLlIy81HQw+btvpVoh6mObvPH28jR4w+aEjIP/EQx8H+Td4WEaZK5IE=
-----END CERTIFICATE-----
)EOF";

#endif
