# Creación y Funcionamiento de Certificados TLS en el Proyecto

## Índice

1. [Introducción](#1-introducción)
2. [Conceptos fundamentales](#2-conceptos-fundamentales)
3. [Infraestructura PKI del proyecto](#3-infraestructura-pki-del-proyecto)
4. [Pasos para generar los certificados](#4-pasos-para-generar-los-certificados)
5. [Estructura de los archivos generados](#5-estructura-de-los-archivos-generados)
6. [Funcionamiento de la comunicación cifrada](#6-funcionamiento-de-la-comunicación-cifrada)
7. [Flujo completo del handshake TLS](#7-flujo-completo-del-handshake-tls)
8. [Cómo se usan los certificados en cada ejercicio](#8-cómo-se-usan-los-certificados-en-cada-ejercicio)
9. [La clase IPBasedWiFiClientSecure](#9-la-clase-ipbasedwificlientsecure)
10. [Dependencia crítica: sincronización NTP](#10-dependencia-crítica-sincronización-ntp)
11. [Verificación de los certificados](#11-verificación-de-los-certificados)
12. [Referencias](#12-referencias)

---

## 1. Introducción

Este documento detalla el proceso completo de creación de la infraestructura de certificados utilizada en la Práctica 05 de Seguridad IoT, donde se implementa una arquitectura de comunicación MQTT entre un ESP32 y un broker EMQX, comparando conexiones sin TLS (Ejercicio 1) vs. con TLS (Ejercicio 2).

Se utiliza una **PKI (Public Key Infrastructure)** de ámbito local auto-gestionada, compuesta por una Autoridad Certificante (CA) raíz que firma un certificado de servidor para el broker EMQX. El ESP32 valida la identidad del broker usando el certificado de la CA incrustado en su firmware.

---

## 2. Conceptos fundamentales

### 2.1. Criptografía híbrida de TLS

TLS combina dos tipos de criptografía en una misma conexión:

| Fase | Tipo de cifrado | Propósito | Algoritmo típico |
|---|---|---|---|
| Handshake | **Asimétrica** | Intercambio seguro de claves + validación de identidad | RSA 2048 bits |
| Datos | **Simétrica** | Transmisión rápida de mensajes | AES (128/256 bits) |

**Ventaja:** se obtiene la seguridad del cifrado asimétrico (intercambio de claves sin compartir secretos previos) con la velocidad del cifrado simétrico (ideal para microcontroladores con recursos limitados).

### 2.2. Certificados digitales X.509

Un certificado X.509 es un documento electrónico que asocia una **clave pública** con una **identidad** (dominio, organización, etc.) y está **firmado digitalmente** por una Autoridad Certificante (CA).

**Contenido principal de un certificado:**
- Clave pública del sujeto (titular)
- Nombre del sujeto (Common Name, CN)
- Nombre del emisor (CA que lo firmó)
- Período de validez (fecha inicio / fecha fin)
- Número de serie único
- Firma digital de la CA
- Subject Alternative Names (SANs) — extensiones modernas para dominios/IPs adicionales

### 2.3. Cadena de confianza (Chain of Trust)

```
CA Raíz (autofirmada) ──firma──> Certificado de Servidor
       ^                                  ^
       |                                  |
  confianza implícita               se presenta al cliente
       ^                                  ^
       |                                  |
  El cliente tiene              El servidor lo usa para
  el CA certificado              demostrar su identidad
  como "raíz de confianza"
```

---

## 3. Infraestructura PKI del proyecto

### 3.1. Árbol de archivos

```
docker/emqx/certs/
├── ca.crt         # Certificado de la CA (autofirmado) — público
├── ca.key         # Clave privada de la CA — SECRETO
├── ca.srl         # Archivo de numeración de serie (serial)
├── server.crt     # Certificado del servidor EMQX (firmado por la CA) — público
├── server.csr     # Certificate Signing Request (solicitud de firma)
└── server.key     # Clave privada del servidor EMQX — SECRETO
```

### 3.2. Contenido decodificado de cada certificado

#### CA Certificate (`ca.crt`)

```
Certificate:
    Data:
        Version: 1 (0x0)
        Serial Number: 1 (0x1)
    Signature Algorithm: sha256WithRSAEncryption
        Issuer: CN = ISPC_CA
        Validity
            Not Before: May 15 00:00:00 2026 GMT
            Not After : May 15 00:00:00 2027 GMT
        Subject: CN = ISPC_CA
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (2048 bit)
    Signature Algorithm: sha256WithRSAEncryption
    (autofirmado: issuer = subject)
```

**Características:**
- **Sujeto (Subject):** `CN = ISPC_CA`
- **Emisor (Issuer):** `CN = ISPC_CA` (autofirmado, raíz de confianza)
- **Validez:** 365 días (15/05/2026 → 15/05/2027)
- **Algoritmo:** RSA 2048 bits con SHA-256
- **Uso:** Firma de certificados (CA:TRUE, KeyCertSign, CRLSign)

#### Server Certificate (`server.crt`)

```
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number: 2 (0x2)
    Signature Algorithm: sha256WithRSAEncryption
        Issuer: CN = ISPC_CA
        Validity
            Not Before: May 15 00:00:00 2026 GMT
            Not After : May 15 00:00:00 2027 GMT
        Subject: CN = mqtt.local
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (2048 bit)
        X509v3 extensions:
            X509v3 Subject Alternative Name:
                DNS:mqtt.local, IP Address:192.168.100.19
    Signature Algorithm: sha256WithRSAEncryption
    (firmado por CN = ISPC_CA)
```

**Características:**
- **Sujeto (Subject):** `CN = mqtt.local`
- **Emisor (Issuer):** `CN = ISPC_CA` (firmado por la CA raíz)
- **Validez:** 365 días (15/05/2026 → 15/05/2027)
- **Algoritmo:** RSA 2048 bits con SHA-256
- **SAN (Subject Alternative Name):** `DNS:mqtt.local, IP:192.168.100.19`
  - Esta extensión es **crítica** porque permite que el cliente valide el certificado tanto contra el nombre de host `mqtt.local` como contra la dirección IP `192.168.100.19`

### 3.3. Archivo de serial (`ca.srl`)

```
01
```

> **Nota:** Con el enfoque `openssl ca` (ver sección 4), OpenSSL gestiona los seriales a través de una base de datos (`index.txt`) y un archivo `serial.txt` que comienza en `01`. El archivo `ca.srl` tradicional no se genera; en su lugar se utiliza `demoCA/serial`.

Es un número hexadecimal de 20 bytes (160 bits) que OpenSSL asigna como **número de serie único** del certificado de servidor. Cada nuevo certificado firmado por esta CA incrementa o genera un nuevo serial, garantizando que cada certificado tenga un identificador único.

---

## 4. Pasos para generar los certificados

Todos los comandos se ejecutan con **OpenSSL** en una terminal Linux/Mac/WSL.

### 4.1. Crear directorio de trabajo

```bash
mkdir -p docker/emqx/certs
cd docker/emqx/certs
```

### 4.2. Generar la clave privada de la CA (2048 bits RSA)

```bash
openssl genrsa -out ca.key 2048
```

- `genrsa` — genera un par de claves RSA
- `-out ca.key` — archivo de salida para la clave privada
- `2048` — tamaño de la clave en bits (estándar mínimo seguro actual)
- **Protección:** Este archivo es el secreto más sensible de la PKI. Quien tenga `ca.key` puede firmar certificados falsos que cualquier dispositivo que confíe en `ca.crt` aceptará como válidos.

### 4.3. Preparar la base de datos de la CA

El comando `openssl ca` requiere una estructura de base de datos para gestionar los certificados emitidos:

```bash
mkdir -p demoCA/newcerts
touch demoCA/index.txt
echo '01' > demoCA/serial
```

Crear archivo de configuración mínimo:

```bash
cat > demoCA/ca.conf << 'EOF'
[ ca ]
default_ca = CA_default

[ CA_default ]
dir = ./demoCA
certs = $dir
crl_dir = $dir/crl
database = $dir/index.txt
new_certs_dir = $dir/newcerts
certificate = $dir/ca.crt
serial = $dir/serial
private_key = $dir/ca.key
RANDFILE = $dir/private/.rand
default_md = sha256
default_days = 365
preserve = no
policy = policy_anything

[ policy_anything ]
commonName = supplied
EOF
```

### 4.4. Generar la clave privada de la CA (2048 bits RSA)

```bash
openssl genrsa -out ca.key 2048
```

- `genrsa` — genera un par de claves RSA
- `-out ca.key` — archivo de salida para la clave privada
- `2048` — tamaño de la clave en bits (estándar mínimo seguro actual)
- **Protección:** Este archivo es el secreto más sensible de la PKI. Quien tenga `ca.key` puede firmar certificados falsos que cualquier dispositivo que confíe en `ca.crt` aceptará como válidos.

### 4.5. Generar el CSR de la CA y autofirmarlo con fecha específica

Se genera un CSR para la CA y luego se autofirma usando `openssl ca`, lo que permite fijar las fechas de validez con `-startdate` y `-enddate`:

```bash
# Generar el CSR de la CA
openssl req -new -key ca.key -out ca.csr -subj "/CN=ISPC_CA"

# Autofirmar con fechas específicas (15/05/2026 → 15/05/2027)
openssl ca -selfsign -keyfile ca.key -cert ca.crt \
  -startdate 20260515000000Z -enddate 20270515000000Z \
  -out ca.crt -in ca.csr -config demoCA/ca.conf -batch
```

| Parámetro | Significado |
|---|---|
| `req -new` | Genera un Certificate Signing Request |
| `-key ca.key` | Clave privada de la CA |
| `-out ca.csr` | CSR generado |
| `-subj "/CN=ISPC_CA"` | Sujeto: Common Name = `ISPC_CA` |
| `ca -selfsign` | Autofirma el CSR (se convierte en CA raíz) |
| `-keyfile ca.key` | Clave privada para firmar |
| `-startdate 20260515000000Z` | Fecha de inicio de validez (15 de mayo 2026) |
| `-enddate 20270515000000Z` | Fecha de fin de validez (15 de mayo 2027) |
| `-config demoCA/ca.conf` | Archivo de configuración de la CA |
| `-batch` | Sin preguntas interactivas |

**Nota:** Se usa `openssl ca` en lugar del simple `openssl req -new -x509` porque este último no permite especificar la fecha de inicio (`-startdate`). La opción `-startdate` y `-enddate` del comando `openssl ca` permite generar certificados con fechas retroactivas o futuras arbitrarias, útil para entornos de laboratorio o pruebas.

**Nota adicional:** Se usa solo `CN` sin `O` (organización) ni `C` (país) porque es una CA de laboratorio/educativa. En producción se incluirían más campos.

### 4.6. Generar la clave privada del servidor EMQX

```bash
openssl genrsa -out server.key 2048
```

- Clave privada RSA 2048 bits para el certificado del broker
- **Protección:** EMQX usará esta clave para demostrar su identidad durante el handshake TLS

### 4.7. Generar el CSR (Certificate Signing Request) del servidor

```bash
openssl req -new -key server.key -out server.csr \
  -subj "/CN=mqtt.local"
```

- `req -new` — crea una solicitud de firma
- `-key server.key` — clave pública se extrae de esta clave privada
- `-out server.csr` — archivo CSR generado
- `-subj "/CN=mqtt.local"` — Common Name = `mqtt.local` (nombre DNS del broker)

**El CSR contiene:**
- La clave pública del servidor (extraída de `server.key`)
- El nombre del sujeto (`CN=mqtt.local`)
- La solicitud firmada con la clave privada del servidor (prueba de posesión)

### 4.8. Firmar el certificado del servidor con la CA raíz

```bash
openssl ca -startdate 20260515000000Z -enddate 20270515000000Z \
  -keyfile ca.key -cert ca.crt \
  -in server.csr -out server.crt \
  -config demoCA/ca.conf -batch \
  -extfile <(printf "subjectAltName=DNS:mqtt.local,IP:192.168.100.19")
```

| Parámetro | Significado |
|---|---|
| `ca` | Comando de Autoridad Certificante de OpenSSL |
| `-startdate 20260515000000Z` | Fecha de inicio de validez (15/05/2026) |
| `-enddate 20270515000000Z` | Fecha de fin de validez (15/05/2027) |
| `-keyfile ca.key` | Clave privada de la CA para firmar |
| `-cert ca.crt` | Certificado de la CA (autoridad firmante) |
| `-in server.csr` | CSR del servidor a firmar |
| `-out server.crt` | Certificado firmado resultante |
| `-config demoCA/ca.conf` | Configuración de la base de datos de la CA |
| `-batch` | Sin preguntas interactivas |
| `-extfile <(...)` | Extensiones X.509 adicionales (SAN) |

**Explicación de la extensión SAN (Subject Alternative Name):**

La línea `printf "subjectAltName=DNS:mqtt.local,IP:192.168.100.19"` agrega una extensión crítica que le dice al cliente:
- "Este certificado es válido para el nombre de host `mqtt.local`"
- "Este certificado también es válido para la dirección IP `192.168.100.19`"

**¿Por qué es necesaria esta extensión?**
- Los navegadores modernos y librerías TLS **ignoran el Common Name (CN)** para la validación de identidad; solo usan las SAN
- El ESP32 se conecta a una **IP fija** (`192.168.100.19`) y no a un nombre DNS
- Sin incluir `IP:192.168.100.19` en las SAN, la validación TLS fallaría porque el CN `mqtt.local` no coincide con la IP de conexión

---

## 5. Estructura de los archivos generados

### 5.1. Formato PEM

Todos los archivos están en formato **PEM (Privacy Enhanced Mail)**, que es texto plano codificado en Base64 con cabeceras y pies de página:

```
-----BEGIN CERTIFICATE-----
(base64 de la estructura DER del certificado)
-----END CERTIFICATE-----
```

### 5.2. Tamaños típicos

| Archivo | Tamaño aprox. | Contenido |
|---|---|---|
| `ca.key` | ~1.7 KB | Clave privada RSA 2048 en PEM |
| `ca.crt` | ~3.6 KB | Certificado X.509 v3 en PEM |
| `ca.srl` | ~0 bytes (no usado) | Serial gestionado via `demoCA/serial` |
| `server.key` | ~1.7 KB | Clave privada RSA 2048 en PEM |
| `server.csr` | ~0.9 KB | CSR en PEM |
| `server.crt` | ~4.1 KB | Certificado X.509 v3 con SAN en PEM |

### 5.3. Ubicación en el contenedor Docker

El `docker-compose.yml` monta el directorio local de certificados dentro del contenedor EMQX:

```yaml
volumes:
  - ./emqx/certs:/opt/emqx/etc/certs
```

EMQX los referencia mediante variables de entorno:

```yaml
environment:
  - EMQX_LISTENERS__SSL__DEFAULT__SSL_OPTIONS__KEYFILE=/opt/emqx/etc/certs/server.key
  - EMQX_LISTENERS__SSL__DEFAULT__SSL_OPTIONS__CERTFILE=/opt/emqx/etc/certs/server.crt
  - EMQX_LISTENERS__SSL__DEFAULT__SSL_OPTIONS__CACERTFILE=/opt/emqx/etc/certs/ca.crt
```

Esto configura el **listener SSL** en el puerto **8883** para que use `server.key` y `server.crt`, y presente `ca.crt` (opcional) como parte de la cadena.

---

## 6. Funcionamiento de la comunicación cifrada

### 6.1. Ejercicio 1 — MQTT sin TLS (texto plano)

**Archivo:** `firmware_ej1_sintls/src/main.cpp`

**Conexión:**
```cpp
WiFiClient espClient;               // Cliente TCP simple (SIN cifrado)
PubSubClient client(espClient);     // MQTT sobre TCP plano
// ...
client.setServer(MQTT_BROKER, 1883); // Puerto 1883 (MQTT no seguro)
```

**Flujo de datos:**
```
ESP32 ──WiFiClient (TCP)──> EMQX :1883
         │
         ├── CONNECT → usuario: "alumno_ispc", contraseña: "mqtt123"
         ├── PUBLISH → topic: "sensores/temperatura"
         │             payload: {"device":"esp32_lab","temperature":24.7}
         └── Todo viaja en TEXTO PLANO
```

**En Wireshark (filtro: `mqtt` o `tcp.port == 1883`):**
- Se ve el paquete CONNECT con usuario y contraseña legibles
- Se ve el paquete PUBLISH con el JSON de temperatura completo
- **No hay protección alguna**

### 6.2. Ejercicio 2 — MQTT con TLS (MQTTS)

**Archivo:** `firmware_ej2_contls/src/main.cpp`

**Conexión:**
```cpp
#include <WiFiClientSecure.h>

IPBasedWiFiClientSecure espClientSecure;  // Cliente con TLS
espClientSecure.setCACert(ca_cert);       // Inyecta CA raíz de confianza
PubSubClient client(espClientSecure);     // MQTT sobre TLS
// ...
client.setServer(MQTT_BROKER, 8883);      // Puerto 8883 (MQTT seguro)
```

**Flujo de datos:**
```
ESP32 ──WiFiClientSecure (TLS)──> EMQX :8883
         │
         ├── 1. Handshake TLS (negociación cifrada)
         ├── 2. Validación del certificado del servidor
         ├── 3. Intercambio de claves de sesión
         ├── 4. CONNECT cifrado → usuario/contraseña OCULTOS
         └── 5. PUBLISH cifrado → payload OCULTO
```

**En Wireshark (filtro: `tcp.port == 8883`):**
- Se ve el handshake TLS (Client Hello, Server Hello, Certificate, etc.)
- Los mensajes MQTT aparecen como `TLS Application Data` cifrados
- **No es posible** leer credenciales ni payload

---

## 7. Flujo completo del handshake TLS

El handshake TLS 1.2 es el proceso de negociación que ocurre ANTES de que cualquier mensaje MQTT se transmita:

```
CLIENTE (ESP32)                        SERVIDOR (EMQX)
      |                                      |
      |  1. Client Hello                      |
      |  ──────────────────────────────────>  |
      |  (versiones TLS, cipher suites sop.)  |
      |                                      |
      |  2. Server Hello                      |
      |  <──────────────────────────────────  |
      |  (versión TLS elegida, cipher suite)  |
      |                                      |
      |  3. Certificate                       |
      |  <──────────────────────────────────  |
      |  (server.crt completo + cadena)       |
      |                                      |
      |  4. Server Hello Done                 |
      |  <──────────────────────────────────  |
      |                                      |
      |  ▼ Validación del certificado:        |
      |    • ¿Firmado por ISPC_CA?            |
      |      → Extrae clave pública de ca_cert|
      |      → Verifica firma del server.crt  |
      |    • ¿Dentro de fecha válida?         |
      |      → Requiere hora sincronizada NTP |
      |    • ¿SAN contiene "mqtt.local"       |
      |        o IP:192.168.100.19?           |
      |                                      |
      |  5. Client Key Exchange               |
      |  ──────────────────────────────────>  |
      |  (pre-master secret cifrado con       |
      |   clave pública del servidor)         |
      |                                      |
      |  6. Change Cipher Spec                |
      |  ──────────────────────────────────>  |
      |  (a partir de aquí, el cliente        |
      |   enviará cifrado)                    |
      |                                      |
      |  7. Finished (cliente)                |
      |  ──────────────────────────────────>  |
      |  (primer mensaje cifrado con clave    |
      |   de sesión)                          |
      |                                      |
      |  8. Change Cipher Spec                |
      |  <──────────────────────────────────  |
      |  (a partir de aquí, el servidor       |
      |   enviará cifrado)                    |
      |                                      |
      |  9. Finished (servidor)               |
      |  <──────────────────────────────────  |
      |                                      |
  ════════════════════════════════════════════════════
  SESIÓN TLS ESTABLECIDA — TODO CIFRADO CON AES
  ════════════════════════════════════════════════════
      |                                      |
      |  MQTT CONNECT (cifrado)               |
      |  ──────────────────────────────────>  |
      |  ("alumno_ispc", "secreto_mqtt_123")  |
      |                                      |
      |  MQTT PUBLISH (cifrado)               |
      |  ──────────────────────────────────>  |
      |  ("sensores/temperatura", JSON)       |
      |                                      |
```

### 7.1. Detalle de la validación del certificado (paso 4)

El ESP32, gracias a `setCACert(ca_cert)`, realiza estas comprobaciones:

1. **Verificación de la firma:** Usa la clave pública de `ca_cert` para verificar que la firma digital de `server.crt` fue creada con la clave privada de la CA (`ca.key`)
2. **Caducidad:** Comprueba que la fecha actual está dentro del rango `Not Before` / `Not After` del certificado
3. **Nombre del servidor (SNI):** Verifica que el nombre al que se conecta (`mqtt.local` o la IP) coincide con alguna de las SAN del certificado

### 7.2. Intercambio de claves (paso 5)

En RSA (el método usado aquí):
1. El ESP32 genera un número aleatorio llamado **pre-master secret**
2. Lo cifra con la **clave pública del servidor** (extraída de `server.crt`)
3. Solo el servidor (que tiene `server.key`, la clave privada correspondiente) puede descifrarlo
4. Ambas partes usan el pre-master secret para derivar las **claves de sesión simétricas**

En ECDHE (alternativa más moderna no usada aquí):
- Se usa Diffie-Hellman sobre curvas elípticas
- Aporta **Perfect Forward Secrecy (PFS)**: si la clave privada del servidor se compromete en el futuro, las sesiones pasadas no pueden descifrarse

---

## 8. Cómo se usan los certificados en cada ejercicio

### 8.1. En el broker EMQX (Docker)

El `docker-compose.yml` monta los certificados y configura el listener SSL:

```yaml
volumes:
  - ./emqx/certs:/opt/emqx/etc/certs
environment:
  - EMQX_LISTENERS__SSL__DEFAULT__SSL_OPTIONS__KEYFILE=/opt/emqx/etc/certs/server.key
  - EMQX_LISTENERS__SSL__DEFAULT__SSL_OPTIONS__CERTFILE=/opt/emqx/etc/certs/server.crt
  - EMQX_LISTENERS__SSL__DEFAULT__SSL_OPTIONS__CACERTFILE=/opt/emqx/etc/certs/ca.crt
```

- **server.key** → clave privada para descifrar el pre-master secret
- **server.crt** → certificado que se envía al cliente durante el handshake
- **ca.crt** → se envía opcionalmente para ayudar al cliente a construir la cadena de confianza

### 8.2. En el ESP32 (Ejercicio 1 — sin TLS)

```cpp
WiFiClient espClient;
```

- No se usa ningún certificado
- La comunicación viaja en TCP plano
- No hay validación de identidad del servidor

### 8.3. En el ESP32 (Ejercicio 2 — con TLS)

En `firmware_ej2_contls/include/secrets.h`:

```cpp
const char *ca_cert = R"EOF(-----BEGIN CERTIFICATE-----
MIIDBTCCAe2gAwIBAgIUEVrdKhuBTJfWifHg6hDgthmcmuQwDQYJKoZIhvcNAQEL
...
-----END CERTIFICATE-----)EOF";
```

- El **certificado completo de la CA** (`ca.crt`) se incrusta como un string RAW en el código
- `espClientSecure.setCACert(ca_cert)` lo carga en la memoria del ESP32 como raíz de confianza
- **No se necesita** la clave privada de la CA ni la del servidor en el dispositivo IoT
- El ESP32 solo necesita el certificado CA para **verificar** la firma del servidor

---

## 9. La clase IPBasedWiFiClientSecure

En `firmware_ej2_contls/src/main.cpp` (líneas 11-31) se define una clase personalizada:

```cpp
class IPBasedWiFiClientSecure : public WiFiClientSecure {
public:
  int connect(const char *host, uint16_t port) override {
    if (strcmp(host, MQTT_BROKER) == 0 || strcmp(host, "mqtt.local") == 0) {
      IPAddress ip;
      ip.fromString(MQTT_BROKER);
      return WiFiClientSecure::connect(ip, port, "mqtt.local", _CA_cert, _cert, _private_key);
    }
    return WiFiClientSecure::connect(host, port);
  }
};
```

### 9.1. ¿Por qué es necesaria?

**El problema:**
- El certificado del servidor tiene `CN = mqtt.local` (nombre DNS, no IP)
- El ESP32 se conecta a `192.168.100.19` (porque en una LAN no hay DNS que resuelva `mqtt.local`)
- La validación TLS por defecto compara el `host` de conexión con el CN/SAN del certificado
- Si te conectas por IP pero el certificado dice `mqtt.local`, la validación **falla**

**La solución:**
- Esta clase sobreescribe `connect()` para:
  1. Convertir la IP string a objeto `IPAddress`
  2. Llamar al método `WiFiClientSecure::connect(ip, port, "mqtt.local", ...)` que:
     - Conecta el socket TCP a la IP `192.168.100.19`
     - Pero valida el certificado contra el nombre `"mqtt.local"` (el CN del certificado)
     - Usa el CA certificado ya cargado (`_CA_cert`) para la validación

- Alternativamente, la extensión SAN con `IP:192.168.100.19` permite que la validación TLS funcione incluso conectando directamente por IP, pero la clase personalizada asegura la compatibilidad.

### 9.2. Sobrecarga adicional

También se sobrecarga la versión con timeout:

```cpp
int connect(const char *host, uint16_t port, int32_t timeout) {
  // misma lógica
}
```

---

## 10. Dependencia crítica: sincronización NTP

En `firmware_ej2_contls/src/main.cpp` (líneas 95-107 y 120-122):

```cpp
void esperarSincronizacionHora() {
  Serial.print("Esperando sincronización de hora NTP...");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  Serial.println("\nHora sincronizada: ");
  Serial.println(asctime(&timeinfo));
}

// En setup():
configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
esperarSincronizacionHora();
```

### 10.1. ¿Por qué es necesario?

La validación TLS requiere verificar que la fecha actual esté dentro del rango de validez del certificado:
- `Not Before: May 15 00:00:00 2026`
- `Not After: May 15 00:00:00 2027`

**Problema típico en IoT:**
- Al encender un ESP32, la hora interna es `0` (1 de enero de 1970, época Unix)
- Si se intenta la conexión TLS inmediatamente, la fecha `1970` está fuera del rango de validez del certificado
- TLS falla con error de validación aunque el certificado sea correcto

**Solución:**
- `configTime()` inicia la sincronización NTP con `pool.ntp.org`
- El bucle `while (now < 24*3600)` espera **bloqueando** hasta que el tiempo Unix supere las 24 horas (es decir, se haya recibido la hora real de NTP)
- Solo entonces se permite que `reconnect()` intente la conexión TLS

### 10.2. Comparación con el Ejercicio 1

```cpp
// Ejercicio 1 (sin TLS)
configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
// No hay espera ni validación de hora

// Ejercicio 2 (con TLS)
configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
esperarSincronizacionHora();  // BLOQUEANTE
```

En el Ejercicio 1, NTP se usa solo para el timestamp del payload JSON. En el Ejercicio 2, es **obligatorio** para que TLS funcione.

---

## 11. Verificación de los certificados

Se pueden inspeccionar y verificar los certificados con OpenSSL:

### 11.1. Ver el contenido del CA

```bash
openssl x509 -in ca.crt -text -noout
```

### 11.2. Ver el contenido del certificado del servidor

```bash
openssl x509 -in server.crt -text -noout
```

### 11.3. Verificar que el server.crt está firmado por la CA

```bash
openssl verify -CAfile ca.crt server.crt
```

Salida esperada:
```
server.crt: OK
```

### 11.4. Verificar la coincidencia de clave privada y certificado

```bash
openssl pkey -in server.key -pubout -outform pem | sha256sum
openssl x509 -in server.crt -pubkey -noout -outform pem | sha256sum
```

(Ambos comandos deben producir el mismo hash, confirmando que server.key y server.crt son un par válido)

### 11.5. Ver el CSR

```bash
openssl req -in server.csr -text -noout
```

---

## 12. Referencias

| Recurso | Enlace |
|---|---|
| OpenSSL Documentation | https://www.openssl.org/docs/ |
| EMQX SSL/TLS Configuration | https://docs.emqx.com/en/emqx/latest/network/ssl-tls.html |
| WiFiClientSecure (ESP32 Arduino) | https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiClientSecure |
| MQTT Security Guide | https://www.emqx.com/en/blog/guide-to-mqtt-security |
| X.509 Certificate Basics | https://letsencrypt.org/es/how-it-works/ |
| TLS Handshake Explained | https://tls13.xargs.org/ |
| ESP32 NTP Time Sync | https://docs.espressif.com/projects/arduino-esp32/en/latest/api/time.html |

---

> **Documento generado para la Práctica 05 — Seguridad IoT**  
> Tecnicatura Superior en Telecomunicaciones — ISPC
