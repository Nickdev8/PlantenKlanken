#include <LiquidCrystal.h>
#include <DHT.h>

// ================== PIN DEFINITIONS ==================
#define PLANT_PIN   A0
#define MOIST_PIN   A2
#define MOIST_PWR   8
#define LDR_PIN     A1

#define DHTPIN      7
#define DHTTYPE     DHT11

#define SPEAKER_PIN 9

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
DHT dht(DHTPIN, DHTTYPE);

// ================== TIMING ==================
const unsigned long PLANT_INTERVAL = 20;
const unsigned long MOIST_INTERVAL = 1000;
const unsigned long MOIST_WARMUP   = 10;

// ================== ADC & FILTERING ==================
const uint8_t ADC_SAMPLES = 8;

// Baseline drift tracking (DC removal)
const float BASELINE_ALPHA = 0.003f;

// AC smoothing
const float SIGNAL_ALPHA   = 0.25f;

// ================== AUDIO ==================
const int PITCH_MIN = 220;
const int PITCH_MAX = 880;

const int SILENCE_THRESHOLD = 6;
const int PITCH_DEADBAND    = 10;

// ================== CLIP LIMITS ==================
const int CLIP_LOW  = 10;
const int CLIP_HIGH = 1013;

// ================== STATE ==================
unsigned long lastPlantRead = 0;
unsigned long lastMoistRead = 0;
unsigned long moistPowerOn  = 0;
bool moistPowerActive = false;

int rawMoisture = 0;
int lightFiltered = 0;

float temperature = 0.0;
float humidity = 0.0;

// AD620 processing
float baseline = 512.0f;
float acSmoothed = 0.0f;
int lastPitch = 0;

// ================== HELPERS ==================
int readAdcAvg(uint8_t pin) {
  unsigned long sum = 0;
  for (uint8_t i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(pin);
  }
  return (int)(sum / ADC_SAMPLES);
}

void calibrateBaseline() {
  unsigned long start = millis();
  unsigned long sum = 0;
  unsigned int count = 0;

  while (millis() - start < 2000) {
    sum += readAdcAvg(PLANT_PIN);
    count++;
    delay(5);
  }

  if (count > 0) {
    baseline = (float)sum / (float)count;
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  pinMode(MOIST_PWR, OUTPUT);
  digitalWrite(MOIST_PWR, LOW);

  pinMode(SPEAKER_PIN, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Calibrating...");
  dht.begin();

  calibrateBaseline();

  lcd.clear();
  lcd.print("Plant monitor");
}

// ================== LOOP ==================
void loop() {
  unsigned long now = millis();

  handlePlantSignal(now);
  handleMoistureAndClimate(now);
}

// ================== FAST: AD620 + AUDIO ==================
void handlePlantSignal(unsigned long now) {
  if (now - lastPlantRead < PLANT_INTERVAL) return;
  lastPlantRead = now;

  int raw = readAdcAvg(PLANT_PIN);

  // Baseline tracking (DC)
  baseline += BASELINE_ALPHA * ((float)raw - baseline);

  // AC signal
  float ac = (float)raw - baseline;
  acSmoothed = acSmoothed * (1.0f - SIGNAL_ALPHA) + ac * SIGNAL_ALPHA;

  bool clipping = (raw <= CLIP_LOW || raw >= CLIP_HIGH);

  // Light sensor
  int lightRaw = readAdcAvg(LDR_PIN);
  lightFiltered = (int)(lightFiltered * 0.8f + lightRaw * 0.2f);

  // ---------- SERIAL OUTPUT ----------
  Serial.print("raw:");
  Serial.print(raw);
  Serial.print(",base:");
  Serial.print((int)baseline);
  Serial.print(",ac:");
  Serial.print((int)acSmoothed);

  // reference lines for Serial Plotter
  Serial.print(",zero:0");
  Serial.print(",top:1000");

  Serial.print(",clip:");
  Serial.print(clipping ? 1 : 0);
  Serial.print(",light:");
  Serial.print(lightFiltered);
  Serial.print(",moisture:");
  Serial.print(rawMoisture);
  Serial.print(",temp:");
  Serial.print(temperature);
  Serial.print(",lucht_vocht:");
  Serial.print(humidity);
  Serial.println();

  // ---------- AUDIO ----------
  int magnitude = abs((int)acSmoothed);

  if (clipping || magnitude < SILENCE_THRESHOLD) {
    noTone(SPEAKER_PIN);
    return;
  }

  int pitch = map(magnitude, 0, 200, PITCH_MIN, PITCH_MAX);
  pitch = constrain(pitch, PITCH_MIN, PITCH_MAX);

  if (abs(pitch - lastPitch) > PITCH_DEADBAND) {
    tone(SPEAKER_PIN, pitch);
    lastPitch = pitch;
  }
}

// ================== SLOW: MOISTURE + DHT ==================
void handleMoistureAndClimate(unsigned long now) {
  if (!moistPowerActive && now - lastMoistRead >= MOIST_INTERVAL) {
    lastMoistRead = now;
    moistPowerActive = true;
    moistPowerOn = now;
    digitalWrite(MOIST_PWR, HIGH);
  }

  if (moistPowerActive && now - moistPowerOn >= MOIST_WARMUP) {
    rawMoisture = analogRead(MOIST_PIN);
    digitalWrite(MOIST_PWR, LOW);
    moistPowerActive = false;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
      humidity = h;
      temperature = t;
    }

    lcd.setCursor(0, 0);
    lcd.print("Temp ");
    lcd.print(temperature, 1);
    lcd.print((char)223);
    lcd.print("C   ");

    lcd.setCursor(0, 1);
    lcd.print("Aard Vocht:");
    lcd.print(rawMoisture);
    lcd.print("   ");
  }
}
