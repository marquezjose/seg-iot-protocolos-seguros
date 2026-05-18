# 📊 Guía de Medición de Parámetros Comparativos

## MQTT sin TLS vs. MQTT con TLS (MQTTS)

> **Asignatura:** Seguridad IoT — Práctico Clase 05  
> **Objetivo:** Documentar las herramientas, funciones y procedimientos para obtener cada parámetro de la tabla comparativa entre comunicación MQTT plana y MQTT con cifrado TLS, usando métricas reales del microcontrolador **ESP32** y capturas de red con **Wireshark**.

---

## 📋 Tabla de parámetros a completar

| Parámetro | MQTT sin TLS | MQTT con TLS |
| :--- | :---: | :---: |
| **Latencia** | _a medir_ | _a medir_ |
| **Consumo de RAM** | _a medir_ | _a medir_ |
| **Uso de CPU** | _a medir_ | _a medir_ |
| **Tamaño de paquetes** | _a medir_ | _a medir_ |
| **Seguridad** | _a evaluar_ | _a evaluar_ |
| **Exposición de credenciales** | _a evaluar_ | _a evaluar_ |
| **Exposición de payload** | _a evaluar_ | _a evaluar_ |

---

## 🛠️ Resumen de herramientas por parámetro

| Parámetro | Herramienta | Función / Filtro |
| :--- | :--- | :--- |
| Latencia | ESP32 — Serial Monitor | `micros()` antes/después de `connect()` y `publish()` |
| Consumo de RAM | ESP32 — Serial Monitor | `ESP.getFreeHeap()`, `ESP.getHeapSize()`, `ESP.getMinFreeHeap()` |
| Uso de CPU | ESP32 — Serial Monitor | `micros()` del ciclo de loop (Opción A) |
| Tamaño de paquetes | Wireshark (PC) | Columna **Length** — filtros `mqtt` / `tcp.port == 8883` |
| Seguridad | Wireshark (PC) | Observar protocolo: ¿MQTT legible o TLS cifrado? |
| Exposición de credenciales | Wireshark (PC) | `mqtt.msgtype == 1` → ¿se ven user/pass en texto plano? |
| Exposición de payload | Wireshark (PC) | `mqtt.msgtype == 3` → ¿se ve el JSON del sensor? |

---

## 1️⃣ Latencia — `micros()` en el ESP32

> **¿Por qué `micros()` y no `millis()`?**  
> `micros()` tiene resolución de **1 µs** (microsegundo), mientras que `millis()` solo tiene resolución de **1 ms**. Dado que algunas operaciones MQTT tardan menos de 1 ms, `micros()` da valores mucho más precisos.

### Qué medir

| Métrica | Descripción |
| :--- | :--- |
| **Latencia de conexión** | Tiempo que tarda `client.connect()` — incluye el handshake TLS en el Ej. 2 |
| **Latencia de publicación** | Tiempo que tarda `client.publish()` |

### Código de instrumentación — `reconnect()`

```cpp
// Dentro de reconnect(), envolver client.connect():
unsigned long startConnect = micros();
if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
  unsigned long connectTime = micros() - startConnect;
  Serial.print("[MÉTRICA] Latencia de conexión: ");
  Serial.print(connectTime);
  Serial.println(" µs");
  Serial.println("conectado");
}
```

### Código de instrumentación — `loop()`

```cpp
// Reemplazar la línea de publish por:
unsigned long startPublish = micros();
client.publish("sensores/temperatura", jsonBuffer, 0);
unsigned long publishTime = micros() - startPublish;

Serial.print("[MÉTRICA] Latencia de publicación: ");
Serial.print(publishTime);
Serial.println(" µs");
```

### Valores de referencia esperados

| | MQTT sin TLS | MQTT con TLS |
| :--- | :---: | :---: |
| **Conexión** | ~5.000 – 50.000 µs | ~500.000 – 2.000.000 µs |
| **Publicación** | ~100 – 500 µs | ~200 – 1.000 µs |

> ⚠️ El handshake TLS es la operación más costosa y se ejecuta **una sola vez** al conectar. Después, los mensajes usan la sesión cifrada ya establecida.

---

## 2️⃣ Consumo de RAM — APIs nativas del ESP32

El ESP32 expone funciones de diagnóstico de memoria a través de la clase `ESP`:

| Función | Qué retorna |
| :--- | :--- |
| `ESP.getFreeHeap()` | Heap libre **actual** (bytes) |
| `ESP.getMinFreeHeap()` | Mínimo heap libre **desde el arranque** (marca de agua baja) |
| `ESP.getHeapSize()` | Heap **total** disponible (bytes) |
| `ESP.getPsramSize()` | PSRAM total (si el módulo lo tiene; sino retorna 0) |

### Función auxiliar reutilizable

```cpp
void printMemoryStats(const char* label) {
  Serial.println("========== MÉTRICAS DE RAM ==========");
  Serial.print("["); Serial.print(label); Serial.println("]");
  Serial.print("  Heap total:       ");
  Serial.print(ESP.getHeapSize());    Serial.println(" bytes");
  Serial.print("  Heap libre:       ");
  Serial.print(ESP.getFreeHeap());    Serial.println(" bytes");
  Serial.print("  Heap mín. libre:  ");
  Serial.print(ESP.getMinFreeHeap()); Serial.println(" bytes");
  Serial.print("  Heap usado:       ");
  Serial.print(ESP.getHeapSize() - ESP.getFreeHeap()); Serial.println(" bytes");
  Serial.println("======================================");
}
```

### Puntos de medición recomendados

```cpp
void setup() {
  Serial.begin(115200);
  printMemoryStats("ANTES de WiFi");       // ① Línea base

  setup_wifi();
  printMemoryStats("DESPUÉS de WiFi");     // ② Costo de WiFi

  client.setServer(MQTT_BROKER, MQTT_PORT);
  // ...
}

void reconnect() {
  // ... después de conectar exitosamente:
  printMemoryStats("DESPUÉS de MQTT connect");  // ③ Costo de MQTT (+TLS)
}

void loop() {
  // ... dentro del bloque de publicación:
  printMemoryStats("DURANTE operación");        // ④ Uso en régimen
}
```

### Valores de referencia esperados

| | MQTT sin TLS | MQTT con TLS |
| :--- | :---: | :---: |
| **Heap usado total** | ~50 – 70 KB | ~90 – 120 KB |
| **Heap libre** | ~250 – 280 KB | ~200 – 230 KB |
| **Overhead de TLS** | — | **~30 – 50 KB extra** |

> 💡 **Dato clave para el informe:** La diferencia de `ESP.getFreeHeap()` entre ambos firmwares en el mismo punto de medición te da el **costo real en RAM** que impone el cifrado TLS.

---

## 3️⃣ Uso de CPU — Métricas del ESP32

El ESP32 no tiene un monitor de CPU tipo `top` como Linux. Sin embargo, hay varias formas de inferir el uso de CPU directamente desde el microcontrolador:

### Opción A — Tiempo de ejecución del loop (✅ recomendada)

La más simple y suficiente para el práctico. Medir cuántos microsegundos consume cada iteración del `loop()`:

```cpp
void loop() {
  unsigned long loopStart = micros();

  // --- todo el contenido del loop ---
  if (!client.connected()) { reconnect(); }
  client.loop();

  // ... bloque de publicación ...

  // --- fin del contenido del loop ---

  unsigned long loopTime = micros() - loopStart;
  Serial.print("[MÉTRICA] Tiempo de ciclo loop: ");
  Serial.print(loopTime);
  Serial.println(" µs");
}
```

> **Interpretación:** más microsegundos = más ciclos de CPU consumidos. La diferencia entre ambos firmwares refleja el overhead del cifrado.

### Opción B — FreeRTOS `vTaskGetRunTimeStats()` (avanzada)

El ESP32 ejecuta FreeRTOS. Esta función reporta el **porcentaje real de CPU por tarea**.

**Requisito:** agregar build flags en `platformio.ini`:

```ini
build_flags = 
  -DCONFIG_FREERTOS_USE_TRACE_FACILITY=1
  -DCONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=1
```

**Código:**

```cpp
void printCpuStats() {
  char buffer[1024];
  vTaskGetRunTimeStats(buffer);
  Serial.println("===== USO DE CPU (FreeRTOS) =====");
  Serial.println("Tarea\t\tTicks\t\t%CPU");
  Serial.println(buffer);
  Serial.println("=================================");
}
```

> ⚠️ Si las build flags causan problemas de compilación, usar la Opción A.

### Opción C — Contador de ciclos idle (intermedia)

Contar cuántas veces el loop puede iterar "en vacío" en un intervalo fijo. Menos iteraciones = más CPU ocupada:

```cpp
volatile unsigned long idleCount = 0;
unsigned long lastIdleCheck = 0;
unsigned long lastIdleCount = 0;

void loop() {
  // ... código normal ...

  idleCount++;

  // Reportar cada 10 segundos
  if (millis() - lastIdleCheck > 10000) {
    unsigned long cycles = idleCount - lastIdleCount;
    Serial.print("[MÉTRICA] Ciclos idle en 10s: ");
    Serial.println(cycles);
    lastIdleCount = idleCount;
    lastIdleCheck = millis();
  }
}
```

### Valores de referencia esperados (Opción A)

| | MQTT sin TLS | MQTT con TLS |
| :--- | :---: | :---: |
| **Loop sin publicación** | ~10 – 50 µs | ~50 – 200 µs |
| **Loop con publicación** | ~500 – 2.000 µs | ~2.000 – 10.000 µs |

---

## 4️⃣ Tamaño de paquetes — Wireshark

### Captura sin TLS (puerto 1883)

```
Filtro Wireshark: mqtt
```

1. Localizar el paquete **CONNECT** → anotar el valor de la columna **Length**
2. Localizar el paquete **PUBLISH** → anotar el valor de la columna **Length**
3. Se pueden usar filtros más específicos:
   - `mqtt.msgtype == 1` → solo paquetes CONNECT
   - `mqtt.msgtype == 3` → solo paquetes PUBLISH

### Captura con TLS (puerto 8883)

```
Filtro Wireshark: tcp.port == 8883
```

1. Localizar los paquetes del **Handshake TLS**: `Client Hello`, `Server Hello`, `Certificate`
2. Localizar los paquetes **Application Data** (contienen el PUBLISH cifrado)
3. Anotar la columna **Length** de cada uno
4. Filtro alternativo para datos de aplicación TLS:
   - `tcp.port == 8883 && tls.record.content_type == 23`

### Valores de referencia esperados

| | MQTT sin TLS | MQTT con TLS |
| :--- | :---: | :---: |
| **CONNECT** | ~80 – 120 bytes | N/A (cifrado dentro de TLS) |
| **PUBLISH** | ~90 – 130 bytes | ~160 – 220 bytes |
| **Handshake TLS** | N/A | ~2.000 – 4.000 bytes (total) |

> 💡 El **overhead** de TLS en cada paquete es de aproximadamente 20-40 bytes (headers TLS record), más el padding del cifrado de bloque.

---

## 5️⃣ Seguridad — Evaluación cualitativa

Este parámetro es **observacional**, no numérico. Se documenta describiendo lo que se observa en Wireshark:

| | MQTT sin TLS | MQTT con TLS |
| :--- | :--- | :--- |
| **Protocolo visible** | `MQTT` (Wireshark decodifica todo) | `TLSv1.2` / `TLSv1.3` |
| **Contenido legible** | Sí — todo en texto plano | No — `Application Data` cifrada |
| **Nivel de seguridad** | Nulo | Alto (cifrado simétrico + asimétrico) |

---

## 6️⃣ Exposición de credenciales — Wireshark (paquete CONNECT)

### Sin TLS

```
Filtro Wireshark: mqtt.msgtype == 1
```

1. Seleccionar el paquete **CONNECT**
2. En el panel de detalle, expandir: `MQ Telemetry Transport Protocol` → `Connect Flags`
3. Se visualiza en **texto plano**:
   - `User Name: alumno_ispc`
   - `Password: mqtt123`
4. 📸 **Tomar captura de pantalla como evidencia**

### Con TLS

```
Filtro Wireshark: tcp.port == 8883
```

1. El paquete CONNECT está **dentro** de un registro TLS `Application Data`
2. Solo se ven bytes hexadecimales cifrados
3. **No es posible** extraer usuario ni contraseña
4. 📸 **Tomar captura de pantalla como evidencia**

---

## 7️⃣ Exposición de payload — Wireshark (paquete PUBLISH)

### Sin TLS

```
Filtro Wireshark: mqtt.msgtype == 3
```

1. Seleccionar el paquete **PUBLISH**
2. Expandir: `MQ Telemetry Transport Protocol` → `Publish Message` → `Payload`
3. Se lee el JSON completo en texto plano:
   ```json
   {"device":"esp32_lab","temperature":25.3,"timestamp":1747612345}
   ```
4. 📸 **Tomar captura de pantalla como evidencia**

### Con TLS

```
Filtro Wireshark: tcp.port == 8883 && tls.record.content_type == 23
```

1. El PUBLISH está cifrado dentro de `Application Data`
2. El contenido son bytes hexadecimales sin interpretación posible
3. **No es posible** leer el payload del sensor
4. 📸 **Tomar captura de pantalla como evidencia**

---

## 📎 Referencias técnicas

| Recurso | Enlace |
| :--- | :--- |
| ESP32 — Documentación `ESP` class | [docs.espressif.com](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/esp.html) |
| FreeRTOS — Runtime Stats | [freertos.org/rtos-run-time-stats](https://www.freertos.org/rtos-run-time-stats.html) |
| Wireshark — Filtros MQTT | [wiki.wireshark.org/MQTT](https://wiki.wireshark.org/MQTT) |
| Wireshark — Filtros TLS | [wiki.wireshark.org/TLS](https://wiki.wireshark.org/TLS) |
| PubSubClient (Arduino MQTT) | [github.com/knolleary/pubsubclient](https://github.com/knolleary/pubsubclient) |

---

> **Nota:** Los valores de referencia son aproximados y pueden variar según el módulo ESP32 específico, la versión del firmware, la calidad de la conexión WiFi y la carga del broker. Lo importante para el informe es la **diferencia relativa** entre ambas configuraciones.
