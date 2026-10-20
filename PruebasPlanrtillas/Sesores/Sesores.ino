#include <DHT.h>

// ==========================================
// 1. DEFINICIÓN DE PINES (ESP32)
// ==========================================
// Pines Digitales
#define DHT_PIN     4   // Pin de datos del DHT11[cite: 30]
#define TRIG_PIN    18  // Pin Trigger del Ultrasónico HC-SR04[cite: 29]
#define ECHO_PIN    19  // Pin Echo del Ultrasónico HC-SR04[cite: 29]

// Pines Analógicos (ADC1 recomendado si se usa Wi-Fi)
#define LDR_PIN     32  // Fotoresistencia[cite: 30]
#define LM35_PIN    33  // Sensor de Temperatura LM35

// ==========================================
// 2. OBJETOS Y CONFIGURACIONES
// ==========================================
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// ==========================================
// 3. FUNCIONES DE LECTURA DE SENSORES
// ==========================================

// --- Sensor de Temperatura y Humedad (DHT11) ---
void leerDHT11(float &temperatura, float &humedad) {
  temperatura = dht.readTemperature();
  humedad = dht.readHumidity();
  
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error: No se pudo leer el sensor DHT11");
    temperatura = -1; // Valores de error
    humedad = -1;
  }
}

// --- Fotoresistencia (LDR) ---
int leerLuzLDR() {
  int valorRaw = analogRead(LDR_PIN);
  
  // El ESP32 tiene un ADC de 12 bits (0 a 4095)[cite: 30].
  // Convertimos a un porcentaje de 0% a 100%
  int porcentajeLuz = map(valorRaw, 0, 4095, 0, 100);
  
  // Opcional: Invertir el mapeo si tu divisor de voltaje está al revés:
  // int porcentajeLuz = map(valorRaw, 0, 4095, 100, 0); 
  
  return porcentajeLuz;
}

// --- Sensor Ultrasónico (HC-SR04) ---
float leerDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // pulseIn lee el tiempo que tarda el pulso en volver
  long duracion = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout de 30ms para evitar cuelgues
  
  if (duracion == 0) {
    return -1.0; // Error o fuera de rango
  }

  // Velocidad del sonido = 343 m/s -> 0.0343 cm/us[cite: 29]
  float distancia_cm = (duracion * 0.0343) / 2.0;
  
  if (distancia_cm > 400 || distancia_cm < 2) {
    return -1.0; // HC-SR04 es fiable entre 2cm y 400cm
  }
  
  return distancia_cm;
}

// --- Sensor Analógico de Temperatura (LM35) ---
float leerLM35() {
  int valorRaw = analogRead(LM35_PIN);
  
  // NOTA CRÍTICA PARA ESP32:
  // La guía menciona (Valor * 5.0 / 1024) * 100 para un Arduino UNO[cite: 24].
  // Sin embargo, el ESP32 trabaja con 3.3V de referencia y un ADC de 4095 pasos (12 bits).
  // La fórmula correcta adaptada para ESP32 es:
  float voltaje = (valorRaw * 3.3) / 4095.0;
  float temperaturaC = voltaje * 100.0; // El LM35 da 10mV por cada 1°C
  
  return temperaturaC;
}

// ==========================================
// CONFIGURACIÓN INICIAL (SETUP)
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // Inicializar pines Ultrasónico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Inicializar pines Analógicos
  pinMode(LDR_PIN, INPUT);
  pinMode(LM35_PIN, INPUT);
  
  // Inicializar DHT
  dht.begin();
  
  Serial.println("Inicialización de Sensores Completada.");
}

// ==========================================
// CICLO PRINCIPAL (LOOP)
// ==========================================
void loop() {
  Serial.println("--- Lectura de Sensores ---");

  // 1. Leer DHT11
  float tempDHT, humDHT;
  leerDHT11(tempDHT, humDHT);
  Serial.print("DHT11 -> Temp: "); Serial.print(tempDHT); Serial.print(" °C | Humedad: "); Serial.print(humDHT); Serial.println(" %");

  // 2. Leer LDR
  int luz = leerLuzLDR();
  Serial.print("LDR -> Iluminación: "); Serial.print(luz); Serial.println(" %");

  // 3. Leer Ultrasónico
  float distancia = leerDistancia();
  Serial.print("HC-SR04 -> Distancia: "); 
  if(distancia != -1.0) {
    Serial.print(distancia); Serial.println(" cm");
  } else {
    Serial.println("Fuera de rango");
  }

  // 4. Leer LM35
  float tempLM35 = leerLM35();
  Serial.print("LM35 -> Temp: "); Serial.print(tempLM35); Serial.println(" °C");

  Serial.println("---------------------------\n");
  
  delay(2000); // Esperar 2 segundos entre lecturas generales
}