#include <WiFi.h>
#include <DHT.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// ==========================================
// 1. CREDENCIALES DE RED Y ADAFRUIT IO
// ==========================================
#define WIFI_SSID       "TU_RED_WIFI"
#define WIFI_PASSWORD   "TU_CONTRASEÑA"

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   
#define AIO_USERNAME    "TU_USUARIO"  
#define AIO_KEY         "TU_LLAVE_SECRETA"           

// ==========================================
// 2. DEFINICIÓN DE PINES (ESP32)
// ==========================================
// Sensores
#define DHT_PIN     4   // DHT11 (Temp/Humedad ambiental)
#define TRIG_PIN    18  // HC-SR04 Trigger (Distancia)
#define ECHO_PIN    19  // HC-SR04 Echo (Distancia)
#define LDR_PIN     32  // LDR (Iluminación ambiental)
#define LM35_PIN    33  // LM35 (Temperatura de la comida)

// Actuadores (LED RGB)
#define PIN_R       25
#define PIN_G       26
#define PIN_B       27

// Configuración PWM para el LED RGB en ESP32
#define PWM_FREQ    5000
#define PWM_RES     8
#define RGB_ANODO_COMUN false // Cambiar a true si tu LED es de ánodo común

// ==========================================
// 3. OBJETOS Y FEEDS DE MQTT
// ==========================================
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Publicaciones (Del ESP32 al Dashboard)
Adafruit_MQTT_Publish feedTempAmb  = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperatura-ambiente");
Adafruit_MQTT_Publish feedLuz      = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/iluminacion");
Adafruit_MQTT_Publish feedNivel    = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/nivel-comida");
Adafruit_MQTT_Publish feedTempPlato= Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperatura-plato");

// Suscripciones (Del Dashboard al ESP32)
Adafruit_MQTT_Subscribe feedBoton  = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/control-luces");

// ==========================================
// 4. VARIABLES GLOBALES
// ==========================================
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

unsigned long tiempoAnterior = 0;
const long INTERVALO_ENVIO = 10000; // 10 segundos (para no saturar Adafruit)

bool lucesHabilitadas = true; // Controlado desde el Dashboard

// ==========================================
// 5. FUNCIONES DE SENSORES Y ACTUADORES
// ==========================================

// Función para cambiar el color del LED RGB
void escribirRGB(uint8_t r, uint8_t g, uint8_t b) {
  if (RGB_ANODO_COMUN) {          
    r = 255 - r;  g = 255 - g;  b = 255 - b;
  }
  ledcWrite(PIN_R, r);
  ledcWrite(PIN_G, g);
  ledcWrite(PIN_B, b);
}

// Lógica inteligente del LED
void actualizarLedInteligente(float distancia, int porcentajeLuz) {
  // 1. Si el usuario apagó el sistema desde la app, apagar y salir.
  if (!lucesHabilitadas) {
    escribirRGB(0, 0, 0); // Apagado
    return;
  }

  // 2. Si está muy oscuro (menos del 20% de luz), encender LED en Blanco para que el gato vea.
  if (porcentajeLuz < 20) {
    escribirRGB(255, 255, 255); // Blanco
    return;
  }

  // 3. Si hay luz normal, el LED indica cuánta comida hay (Distancia)
  if (distancia <= 0) {
    escribirRGB(0, 0, 0); // Error de lectura -> Apagado
  } else if (distancia < 5.0) {
    escribirRGB(0, 255, 0);   // Verde: Plato Lleno (Poca distancia al sensor)
  } else if (distancia < 15.0) {
    escribirRGB(255, 255, 0); // Amarillo: Plato a la mitad
  } else {
    escribirRGB(255, 0, 0);   // Rojo: Plato Vacío (Mucha distancia)
  }
}

float leerUltrasónico() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duracion = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duracion == 0) return -1;
  return (duracion * 0.0343) / 2;
}

// ==========================================
// 6. FUNCIONES DE CONEXIÓN
// ==========================================
void conectarWiFi() {
  Serial.print("\nConectando a Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" Conectado!");
}

void conectarMQTT() {
  if (mqtt.connected()) return;
  Serial.print("Conectando a Adafruit IO... ");
  int8_t ret;
  uint8_t intentos = 3;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    mqtt.disconnect();
    delay(5000);
    if (--intentos == 0) ESP.restart();
  }
  Serial.println("¡Conectado!");
}

// ==========================================
// CONFIGURACIÓN INICIAL (SETUP)
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // Configurar Pines
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(LM35_PIN, INPUT);

  // Configurar PWM para el LED RGB
  ledcAttach(PIN_R, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_G, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_B, PWM_FREQ, PWM_RES);
  escribirRGB(0, 0, 0); // Apagar al inicio

  dht.begin();
  
  mqtt.subscribe(&feedBoton);
  conectarWiFi();
}

// ==========================================
// CICLO PRINCIPAL (LOOP)
// ==========================================
void loop() {
  conectarMQTT();

  // ---------------------------------------------------------
  // A. RECIBIR ÓRDENES DEL DASHBOARD (Suscripción)
  // ---------------------------------------------------------
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(200))) {
    if (subscription == &feedBoton) {
      String mensaje = (char *)feedBoton.lastread;
      Serial.print("Orden Dashboard: "); Serial.println(mensaje);
      
      if (mensaje == "ON") lucesHabilitadas = true;
      if (mensaje == "OFF") lucesHabilitadas = false;
    }
  }

  // ---------------------------------------------------------
  // B. LECTURA DE SENSORES Y CONTROL DE ACTUADOR
  // ---------------------------------------------------------
  // Leer DHT11
  float tempAmbiente = dht.readTemperature();
  
  // Leer LDR (0 a 4095 a Porcentaje 0-100)
  int luzRaw = analogRead(LDR_PIN);
  int luzPorcentaje = map(luzRaw, 0, 4095, 0, 100);
  
  // Leer HC-SR04
  float distancia = leerUltrasónico();
  
  // Leer LM35 (Fórmula corregida para ESP32 de 3.3V)
  float tempPlato = ((analogRead(LM35_PIN) * 3.3) / 4095.0) * 100.0;

  // Actualizar el LED RGB basándose en las lecturas
  actualizarLedInteligente(distancia, luzPorcentaje);

  // ---------------------------------------------------------
  // C. PUBLICAR DATOS AL DASHBOARD (Cada 10 segundos)
  // ---------------------------------------------------------
  if (millis() - tiempoAnterior >= INTERVALO_ENVIO) {
    tiempoAnterior = millis();
    Serial.println("\n--- Enviando datos a Adafruit IO ---");

    if (!isnan(tempAmbiente)) {
      feedTempAmb.publish(tempAmbiente);
      Serial.print("Temp Amb: "); Serial.println(tempAmbiente);
    }
    
    feedLuz.publish(luzPorcentaje);
    Serial.print("Luz: "); Serial.println(luzPorcentaje);
    
    if (distancia > 0) {
      feedNivel.publish(distancia);
      Serial.print("Distancia: "); Serial.println(distancia);
    }
    
    feedTempPlato.publish(tempPlato);
    Serial.print("Temp Plato: "); Serial.println(tempPlato);
  }
}