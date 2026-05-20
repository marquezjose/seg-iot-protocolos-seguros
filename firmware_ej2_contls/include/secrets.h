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
MIIDBTCCAe2gAwIBAgIUEVrdKhuBTJfWifHg6hDgthmcmuQwDQYJKoZIhvcNAQEL
BQAwEjEQMA4GA1UEAwwHSVNQQ19DQTAeFw0yNjA1MjAyMzAxMjdaFw0yNzA1MjAy
MzAxMjdaMBIxEDAOBgNVBAMMB0lTUENfQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IB
DwAwggEKAoIBAQCZMKwWqa2hJOkP0VrIUxbpiwmJhPvQRlcBVGO5XOHwcMA1akTd
Y0aQ86fnABdF6DbV0Pgb11OP3p3uI83QD8c27zK5HGi0IcQSSsV/CWW0Bm4R7906
BonJ1TvbD8ff14dfVeEzx9Nwbvb+42Xb/3TA7qg1ds0/GPj4E6rBVSRNmvcoebjE
sZUBAJ3w3cbG/aYcS2zQXkBM0lPX4ZPX/sHkdbdHvfh0wkFohYbDuauiFo7oj/tr
jhVxGTQ/QbGVjShrJEuqbHexBcIcdUxnazRfQfYBTL9vr3Gps7yJCGs5ZoVLXnXy
2pcaqfb7lXeVyd/fy72VfDIvskEA+F7/GBR/AgMBAAGjUzBRMB0GA1UdDgQWBBQu
qjX9gan52gxS9gDsVV1Ru6EocjAfBgNVHSMEGDAWgBQuqjX9gan52gxS9gDsVV1R
u6EocjAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQA/kD5Qr91D
ikfM+nGJXk3pXTJuX+jKBMwoCb/c3SiTQ89T9Nk6MNoRysUyfaAcHDjKTQlwBXfF
BR2lwjpe2Ro33ooxzzz0SkmezeVeW04nWrqfASlzVyA/VnW0UwRYgm0/jaA5LgQI
wbEq11hmXUpoyDAxouP8deDKC2VyC2AQNmdsyyG6ysMy+115pYOsOLXCWjMXSGZE
V78l9bCatS+SqoS2Mw21cc5Cc0IeXeGsb9S48YmpKmOlBFgtXserZ4TLbY15i4MT
uKOYyZw5xxNfic6IVNMhTg+z6ye3DcsA/Bw3NNpqQ59n3kwl9hSMlJDHi9HZzPyR
YSiZYwfL6VNs-----END CERTIFICATE-----)EOF";

#endif
