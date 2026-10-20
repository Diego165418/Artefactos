#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// ==========================================
// 1. CREDENCIALES DE RED WI-FI
// ==========================================
#define WIFI_SSID       "NOMBRE_DE_TU_RED"
#define WIFI_PASSWORD   "CONTRASEÑA_DE_TU_RED"

// ==========================================
// 2. CREDENCIALES DE ADAFRUIT IO
// ==========================================
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // Puerto estándar MQTT
#define AIO_USERNAME    "TU_USUARIO_ADAFRUIT"  // Reemplazar con tu usuario
#define AIO_KEY         "TU_AIO_KEY"           // Reemplazar con tu llave secreta

// ==========================================
// 3. CONFIGURACIÓN DE CLIENTE Y FEEDS
// ==========================================
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Feeds para PUBLICAR (Enviar datos de sensores al Dashboard)
Adafruit_MQTT_Publish feedTemperatura = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperatura");
Adafruit_MQTT_Publish feedDistancia   = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/distancia");

// Feeds para SUSCRIBIRSE (Recibir órdenes desde botones/switches del Dashboard)
Adafruit_MQTT_Subscribe feedBoton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/boton");

// Variables para control de tiempo (Evitar sobrepasar el límite de Adafruit)
unsigned long tiempoAnterior = 0;
const long INTERVALO_ENVIO = 5000; // 5 segundos (Adafruit permite máx 30 datos por minuto)

// ==========================================
// FUNCIONES DE CONEXIÓN
// ==========================================
void conectarWiFi() {
  Serial.print("Conectando a Wi-Fi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  // Si ya está conectado, salir de la función
  if (mqtt.connected()) return;

  Serial.print("Conectando a Adafruit IO... ");
  int8_t ret;
  uint8_t intentos = 3;
  
  while ((ret = mqtt.connect()) != 0) { // Devuelve 0 si la conexión es exitosa
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Reintentando en 5 segundos...");
    mqtt.disconnect();
    delay(5000);
    if (--intentos == 0) {
      Serial.println("Fallo al conectar. Reiniciando ESP32...");
      ESP.restart(); // Reiniciar placa si falla rotundamente
    }
  }
  Serial.println("¡Conectado a Adafruit IO!");
}

// ==========================================
// CONFIGURACIÓN INICIAL (SETUP)
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // Suscribirse a los feeds entrantes antes de conectar
  mqtt.subscribe(&feedBoton);
  
  conectarWiFi();
}

// ==========================================
// CICLO PRINCIPAL (LOOP)
// ==========================================
void loop() {
  conectarMQTT(); // Asegurar que la conexión MQTT siga activa[cite: 29]

  // 1. ESCUCHAR DATOS DESDE EL DASHBOARD (Suscripciones)
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(200))) {
    if (subscription == &feedBoton) {
      String orden = (char *)feedBoton.lastread;
      Serial.print("Orden recibida del Dashboard: ");
      Serial.println(orden);
      
      // Aquí puedes encender un LED o activar un motor dependiendo de la orden
      if (orden == "ON") {
        // Acción de encendido
      } else if (orden == "OFF") {
        // Acción de apagado
      }
    }
  }

  // 2. ENVIAR DATOS AL DASHBOARD (Publicaciones temporizadas)
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= INTERVALO_ENVIO) {
    tiempoAnterior = tiempoActual;

    // Supongamos que aquí lees tus sensores (ej. float temp = leerSensor();)
    float tempSimulada = 25.5; 
    float distSimulada = 15.0;

    Serial.println("Enviando datos a Adafruit IO...");
    
    // Publicar Temperatura
    if (!feedTemperatura.publish(tempSimulada)) {
      Serial.println("Fallo al publicar Temperatura");
    }
    
    // Publicar Distancia
    if (!feedDistancia.publish(distSimulada)) {
      Serial.println("Fallo al publicar Distancia");
    }
  }

  // Mantener viva la conexión con el servidor si no se envían datos
  // mqtt.ping(); 
}