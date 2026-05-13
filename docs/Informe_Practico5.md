# Informe Técnico: Práctica 05 - Seguridad IoT (MQTT y TLS)

## 1. Arquitectura del Proyecto
La arquitectura implementada consta de las siguientes partes:
- **Dispositivo IoT:** ESP32 simulando un sensor (generación de datos aleatorios con jitter).
- **Protocolo de Comunicación:** MQTT y MQTTS (MQTT sobre TLS).
- **Broker MQTT:** EMQX ejecutándose en un contenedor Docker, configurado con certificados auto-firmados para habilitar cifrado TLS en el puerto 8883.
- **Análisis de Red:** Wireshark.

## 2. Ejercicio 1: Análisis de tráfico MQTT sin TLS (Puerto 1883)
En esta fase, el ESP32 transmite datos hacia el broker en texto plano.

### Evidencias a adjuntar (Wireshark)
- [ ] Captura del paquete `CONNECT` (Evidenciar usuario y contraseña).
- [ ] Captura del paquete `PUBLISH` (Evidenciar el payload JSON visible: `{"device":"esp32_lab", ...}`).

### Notas / Observaciones:
*(Espacio para describir la facilidad con la que se interceptan credenciales cuando no hay cifrado)*

---

## 3. Ejercicio 2: Comunicación MQTT segura con TLS (Puerto 8883)
Se generó una Autoridad Certificante (CA) propia mediante OpenSSL, junto con certificados para el Broker. El ESP32 fue configurado con la clase `WiFiClientSecure` y se le inyectó la CA para verificar la identidad del broker.

### Evidencias a adjuntar (Wireshark)
- [ ] Captura del "Handshake" (`Client Hello`, `Server Hello`, Intercambio de Certificados).
- [ ] Captura del tráfico clasificado como `Application Data` (Evidenciar que ni las credenciales ni el payload son legibles).

---

## 4. Ejercicio 3: Análisis Comparativo (MQTT vs MQTTS)
*(A completar con los resultados observados durante las pruebas)*

| Parámetro | MQTT sin TLS | MQTT con TLS |
| :--- | :--- | :--- |
| **Latencia** | ... | ... |
| **Tamaño de paquetes** | ... | ... |
| **Seguridad** | Nula (Texto plano) | Alta (Cifrado asimétrico/simétrico) |
| **Exposición credenciales**| Totalmente expuestas | Ocultas (Cifradas) |
| **Exposición payload** | Totalmente expuesto | Oculto (Cifrado) |

### Conclusiones
1. **¿Qué información puede verse en MQTT sin TLS?**
   *(Responder aquí...)*
2. **¿Por qué TLS protege las credenciales?**
   *(Responder aquí...)*
3. **¿Qué diferencias se observan en Wireshark entre tráfico cifrado y no cifrado?**
   *(Responder aquí...)*
4. **¿Qué impacto tiene TLS sobre el rendimiento del sistema?**
   *(Responder aquí...)*

---
*Fin del borrador. Adjuntar capturas de pantalla de código (SnapCode) y certificados generados para la entrega final en PDF.*
