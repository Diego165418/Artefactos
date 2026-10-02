float TRIG_PIN = 18;
float ECHO_PIN = 19;

void setup() {

  Serial.begin(115200);

  pinMode(TRIG_PIN , OUTPUT);
  pinMode(ECHO_PIN , INPUT);
}

void loop() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duracion = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duracion == 0) {

    Serial.println("No hay ecoxd");

  } else {

    float d = (0.0343 * duracion)/2;

    Serial.print("la distancia es: ");
    Serial.println(d);
    Serial.println("");
  }

  delay(500);
}