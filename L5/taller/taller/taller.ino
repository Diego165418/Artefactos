#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ==========================================
// CREDENCIALES DE RED Y MQTT
// ==========================================
const char* WIFI_SSID     = "Redmi Note 12 Pro";
const char* WIFI_PASSWORD = "quesito18";
const char* MQTT_SERVER   = "10.61.241.89";
const int MQTT_PORT       = 1883;

// ==========================================
// CONFIGURACIÓN DE SENSORES
// ==========================================
#define DHTPIN 4          // Pin de datos del DHT11
#define DHTTYPE DHT11     // Tipo de sensor DHT
#define LDR_PIN 32        // Pin analógico de la fotoresistencia

DHT dht(DHTPIN, DHTTYPE);
WiFiClient clienteWiFi;
PubSubClient clienteMQTT(clienteWiFi);

unsigned long tiempoAnterior = 0;
const long INTERVALO = 2000; // 2 segundos

// ==========================================
// FUNCIONES DE CONEXIÓN
// ==========================================
void conectarWiFi() {
  Serial.print("Conectando a la red WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConexión WiFi exitosa. IP: ");
  Serial.println(WiFi.localIP());
}

void reconectarMQTT() {
  while (!clienteMQTT.connected()) {
    Serial.print("Conectando al broker Mosquitto...");
    // Generar un ID de cliente aleatorio para evitar conflictos
    String idCliente = "ESP32Cliente-";
    idCliente += String(random(0xffff), HEX);
    
    if (clienteMQTT.connect(idCliente.c_str())) {
      Serial.println(" ¡Conectado!");
    } else {
      Serial.print(" Error de conexión, rc=");
      Serial.print(clienteMQTT.state());
      Serial.println(". Reintentando en 2 segundos...");
      delay(2000);
    }
  }
}

// ==========================================
// CONFIGURACIÓN INICIAL
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // Inicializar sensores
  dht.begin();
  pinMode(LDR_PIN, INPUT);
  
  // Inicializar conexiones
  conectarWiFi();
  clienteMQTT.setServer(MQTT_SERVER, MQTT_PORT);
}

// ==========================================
// CICLO PRINCIPAL
// ==========================================
void loop() {
  // Mantener las conexiones activas
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }
  if (!clienteMQTT.connected()) {
    reconectarMQTT();
  }
  clienteMQTT.loop();

  // Publicar datos cada 2 segundos
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= INTERVALO) {
    tiempoAnterior = tiempoActual;

    // 1. Leer sensores
    float temperatura = dht.readTemperature();
    float humedad = dht.readHumidity();
    int valorLDR = analogRead(LDR_PIN);
    
    // Convertir el valor analógico (0-4095) a un porcentaje (0-100%)
    int porcentajeLuz = map(valorLDR, 0, 4095, 0, 100);

    // 2. Publicar temperatura y humedad si la lectura es válida
    if (!isnan(temperatura) && !isnan(humedad)) {
      clienteMQTT.publish("clima/esp32/temperatura", String(temperatura).c_str());
      clienteMQTT.publish("clima/esp32/humedad", String(humedad).c_str());
      
      Serial.print("Temperatura: ");
      Serial.print(temperatura);
      Serial.print(" °C | Humedad: ");
      Serial.print(humedad);
      Serial.println(" %");
    } else {
      Serial.println("Fallo al leer el sensor DHT11.");
    }

    // 3. Publicar iluminación
    clienteMQTT.publish("clima/esp32/luz", String(porcentajeLuz).c_str());
    Serial.print("Lectura LDR: ");
    Serial.print(valorLDR);
    Serial.print(" | Luz: ");
    Serial.print(porcentajeLuz);
    Serial.println(" %");
    Serial.println("-----------------------------------");
  }
}