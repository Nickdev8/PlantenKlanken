#include <LiquidCrystal.h>
#include <DHT.h>

#define MOIST_PWR 8
#define MOIST_PIN A2
#define PLANT_PIN A0
#define LDR_PIN   A1

#define DHTPIN 7
#define DHTTYPE DHT11

#define SPEAKER_PIN 9

// RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
DHT dht(DHTPIN, DHTTYPE);

// Timing (ms)
const unsigned long plantInterval = 20;
const unsigned long moistInterval = 1000;
const unsigned long moistWarmup   = 10;

unsigned long lastPlantRead = 0;
unsigned long lastMoistRead = 0;

int moistureValue = 0;
float temperature = 0.0;
float humidity = 0.0;

void setup() {
  Serial.begin(115200);

  pinMode(MOIST_PWR, OUTPUT);
  digitalWrite(MOIST_PWR, LOW);

  pinMode(SPEAKER_PIN, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Plant monitor");

  dht.begin();
}

void loop() {
  unsigned long now = millis();

  // ---------- FAST SIGNALS ----------
  if (now - lastPlantRead >= plantInterval) {
    lastPlantRead = now;

    int plant = analogRead(PLANT_PIN);
    int light = analogRead(LDR_PIN);

    // Map plant signal to musical pitch
    int pitch = map(plant, 0, 1023, 220, 880);
    pitch = constrain(pitch, 220, 880);

    tone(SPEAKER_PIN, pitch, 15);

    Serial.print("plant:");
    Serial.print(plant);
    Serial.print(",moisture:");
    Serial.print(moistureValue);
    Serial.print(",light:");
    Serial.print(light);
    Serial.print(",temp:");
    Serial.print(temperature);
    Serial.print(",lucht_vocht:");
    Serial.print(humidity);
    Serial.println();
  }

  // ---------- SLOW SENSORS ----------
  if (now - lastMoistRead >= moistInterval) {
    lastMoistRead = now;

    digitalWrite(MOIST_PWR, HIGH);
    delay(moistWarmup);
    moistureValue = analogRead(MOIST_PIN);
    digitalWrite(MOIST_PWR, LOW);

    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    lcd.setCursor(0, 0);
    lcd.print("Temp ");
    lcd.print(temperature, 1);
    lcd.print((char)223);
    lcd.print("C   ");

    lcd.setCursor(0, 1);
    lcd.print("Aard Vocht:");
    lcd.print(moistureValue);
    lcd.print(" ");
  }
}
