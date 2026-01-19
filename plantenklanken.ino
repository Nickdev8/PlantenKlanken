#define MOIST_PWR  8
#define MOIST_PIN A2
#define PLANT_PIN A0
#define LDR_PIN   A1

// Timing (ms)
const unsigned long plantInterval = 20;     // fast signals
const unsigned long moistInterval = 1000;   // slow moisture read
const unsigned long moistWarmup   = 10;

unsigned long lastPlantRead = 0;
unsigned long lastMoistRead = 0;

int moistureValue = 0;

void setup() {
  Serial.begin(115200);

  pinMode(MOIST_PWR, OUTPUT);
  digitalWrite(MOIST_PWR, LOW);
}

void loop() {
  unsigned long now = millis();

  // ---------- FAST SIGNALS ----------
  if (now - lastPlantRead >= plantInterval) {
    lastPlantRead = now;

    int plant = analogRead(PLANT_PIN);
    int light = analogRead(LDR_PIN);

    // Serial Plotter output
    Serial.print("plant:");
    Serial.print(plant);

    Serial.print(",moisture:");
    Serial.print(moistureValue);

    Serial.print(",light:");
    Serial.print(light);

    // reference lines for stable plotting
    Serial.print(",min:0");
    Serial.print(",max:1000");

    Serial.println();
  }

  // ---------- SLOW MOISTURE SENSOR ----------
  if (now - lastMoistRead >= moistInterval) {
    lastMoistRead = now;

    digitalWrite(MOIST_PWR, HIGH);
    delay(moistWarmup);
    moistureValue = analogRead(MOIST_PIN);
    digitalWrite(MOIST_PWR, LOW);
  }
}
