#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <math.h>
#include <DHT.h>
#include <MAX30105.h>
#include <spo2_algorithm.h>
#include "heartRate.h"
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include "dashboard_page.h"

// =========================
// WiFi
// =========================
const char* WIFI_SSID = "DES";
const char* WIFI_PASSWORD = "88888888";

// =========================
// I2C pins
// =========================
static const int I2C_SDA_PIN = 21;
static const int I2C_SCL_PIN = 22;

// =========================
// DHT11                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
// =========================
#define DHT_PIN 4
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

// =========================
// LCD (20x4, I2C backpack, shares SDA 21 / SCL 22)
// =========================
static const uint8_t LCD_COLS = 20;
static const uint8_t LCD_ROWS = 4;
static const unsigned long LCD_UPDATE_MS = 500;

hd44780_I2Cexp lcd;               // finds the I2C address automatically
bool lcdOk = false;
unsigned long lastLcdMs = 0;
char lcdCache[LCD_ROWS][LCD_COLS + 1];   // remembers what each row shows

// =========================
// Timing
// =========================
static const uint16_t SLOW_SENSOR_MS = 2000;

// =========================
// MAX30102
// =========================
static const long FINGER_IR_THRESHOLD = 50000;
static const int32_t SPO2_BUFFER_LENGTH = 100;

// New readings are calculated every SPO2_SHIFT_SAMPLES new samples.
// At 25 samples/second: 25 = every 1.0 s, 12 = every ~0.5 s
static const int32_t SPO2_SHIFT_SAMPLES = 12;

// How long the finger must be gone before the readings reset
static const unsigned long FINGER_LOST_MS = 1500;

// If no valid reading arrives for this long, go back to "measuring"
static const unsigned long READING_STALE_MS = 8000;

// Smoothing: higher = reacts faster, lower = steadier (0.0 - 1.0)
static const float HR_SMOOTHING = 0.2f;

// How often the displayed heart rate is allowed to change
static const unsigned long HR_DISPLAY_INTERVAL_MS = 2000;   // 2 seconds
static const float SPO2_SMOOTHING = 0.20f;

// A sudden jump bigger than this (BPM) is treated as an outlier
// unless it repeats OUTLIER_ACCEPT_COUNT times in a row
static const int HR_OUTLIER_JUMP = 3;
static const int OUTLIER_ACCEPT_COUNT = 3;

// The heart rate (smoothed value AND the number on the LCD/dashboard)
// is never allowed to move by more than this many BPM in one step
static const int HR_MAX_STEP = 3;

// Number of first readings collected before the first value is shown
static const byte HR_WARMUP_COUNT = 2;

// =========================
// Web server
// =========================
WebServer server(80);

// =========================
// MAX30102 object
// =========================
MAX30105 particleSensor;

// =========================
// Sensor status
// =========================
bool maxOk = false;

// =========================
// Timing variables
// =========================
unsigned long lastSlowMs = 0;
unsigned long lastDebugMs = 0;
unsigned long lastFingerSeenMs = 0;
unsigned long lastValidHrMs = 0;
unsigned long lastValidSpo2Ms = 0;
unsigned long lastBeatMs = 0;
unsigned long lastHrDisplayMs = 0;

// =========================
// DHT11 readings
// =========================
float roomTempC = 0.0f;
float roomHumidity = 0.0f;

// =========================
// Live readings shown on the dashboard (-1 = no reading yet)
// =========================
int spo2Pct = -1;
int heartRateBpm = -1;

bool fingerDetected = false;

// Smoothed values
float hrSmooth = 0.0f;
bool hrHave = false;
int hrOutlierCount = 0;
int hrWarm[HR_WARMUP_COUNT];
byte hrWarmN = 0;

float spo2Smooth = 0.0f;
bool spo2Have = false;

// =========================
// MAX30102 algorithm buffers
// =========================
uint32_t irBuffer[100];
uint32_t redBuffer[100];

int32_t spo2 = 0;
int8_t validSPO2 = 0;

int32_t maximHeartRate = 0;
int8_t validMaximHeartRate = 0;

byte sampleIndex = 0;
bool bufferFilled = false;

// =========================
// JSON buffer
// =========================
char jsonBuffer[400];

// ============================================================
// Reset readings so a new finger starts fresh
// ============================================================
void resetReadings() {

  heartRateBpm = -1;
  spo2Pct = -1;

  hrHave = false;
  spo2Have = false;
  hrOutlierCount = 0;
  hrWarmN = 0;
  lastBeatMs = 0;

  // Throw away stale samples and refill the buffer
  sampleIndex = 0;
  bufferFilled = false;

  validSPO2 = 0;
  validMaximHeartRate = 0;
}

// ============================================================
// Shift MAX30102 buffers
// ============================================================
void shiftBuffers() {

  for (byte i = SPO2_SHIFT_SAMPLES;
       i < SPO2_BUFFER_LENGTH;
       i++) {

    redBuffer[i - SPO2_SHIFT_SAMPLES] = redBuffer[i];
    irBuffer[i - SPO2_SHIFT_SAMPLES] = irBuffer[i];
  }

  sampleIndex = SPO2_BUFFER_LENGTH - SPO2_SHIFT_SAMPLES;
}

// ============================================================
// Feed a new heart-rate value into the live display
// ============================================================
void updateLiveHeartRate(int newBpm) {

  const unsigned long now = millis();

  // First readings: collect a few and start from their median,
  // so one bad first interval cannot become the starting point
  if (!hrHave) {

    hrWarm[hrWarmN++] = newBpm;

    if (hrWarmN < HR_WARMUP_COUNT) {
      return;
    }

    // Median of the three warm-up readings
    int a = hrWarm[0], b = hrWarm[1], c = hrWarm[2];
    int med = max(min(a, b), min(max(a, b), c));

    hrSmooth = med;
    hrHave = true;
    hrWarmN = 0;
    hrOutlierCount = 0;
    lastValidHrMs = now;
    lastHrDisplayMs = now;
    heartRateBpm = med;
    return;
  }

  const float diff = newBpm - hrSmooth;

  // A reading far from the current value is treated as an outlier
  if (fabsf(diff) > HR_OUTLIER_JUMP) {

    hrOutlierCount++;

    if (hrOutlierCount < OUTLIER_ACCEPT_COUNT) {
      return;
    }

    // The change keeps repeating, so it is real. Follow it,
    // but only by HR_MAX_STEP at a time (never a sudden jump)
    hrSmooth += constrain(diff, -(float)HR_MAX_STEP, (float)HR_MAX_STEP);

    // Stay one short of the limit so the next repeated reading
    // takes another step straight away
    hrOutlierCount = OUTLIER_ACCEPT_COUNT - 1;

  } else {

    hrOutlierCount = 0;

    // Smooth gently so the value moves like a watch does
    hrSmooth = hrSmooth + HR_SMOOTHING * diff;
  }

  lastValidHrMs = now;

  // Only change the number on the dashboard every HR_DISPLAY_INTERVAL_MS,
  // and by no more than HR_MAX_STEP each time
  if (heartRateBpm < 0 || now - lastHrDisplayMs >= HR_DISPLAY_INTERVAL_MS) {

    const int target = (int)(hrSmooth + 0.5f);

    if (heartRateBpm < 0) {
      heartRateBpm = target;
    } else {
      heartRateBpm += constrain(target - heartRateBpm, -HR_MAX_STEP, HR_MAX_STEP);
    }

    lastHrDisplayMs = now;
  }
}

// ============================================================
// Feed a new SpO2 value into the live display
// ============================================================
void updateLiveSpO2(int newSpo2) {

  if (!spo2Have) {
    spo2Smooth = newSpo2;
    spo2Have = true;
  } else {
    spo2Smooth = spo2Smooth + SPO2_SMOOTHING * (newSpo2 - spo2Smooth);
  }

  lastValidSpo2Ms = millis();
  spo2Pct = (int)(spo2Smooth + 0.5f);
}

// ============================================================
// Calculate SpO2 and heart rate from the current buffer
// ============================================================
void calculateVitals() {

  maxim_heart_rate_and_oxygen_saturation(
    irBuffer,
    SPO2_BUFFER_LENGTH,
    redBuffer,
    &spo2,
    &validSPO2,
    &maximHeartRate,
    &validMaximHeartRate
  );

  if (!fingerDetected) {
    return;
  }

  

  if (validSPO2 &&
      spo2 >= 92 &&
      spo2 <= 100) {

    updateLiveSpO2((int)spo2);
  }

  // If good readings stop arriving, drop back to "measuring"
  const unsigned long now = millis();

  if (hrHave && now - lastValidHrMs > READING_STALE_MS) {
    hrHave = false;
    hrWarmN = 0;
    heartRateBpm = -1;
  }

  if (spo2Have && now - lastValidSpo2Ms > READING_STALE_MS) {
    spo2Have = false;
    spo2Pct = -1;
  }
}

// ============================================================
// Read MAX30102
// ============================================================
void updateMax30102() {

  if (!maxOk) {
    return;
  }

  particleSensor.check();

  while (particleSensor.available()) {

    const uint32_t irValue = particleSensor.getIR();
    const uint32_t redValue = particleSensor.getRed();

    particleSensor.nextSample();

    // -------------------------
    // Finger detection
    // -------------------------
    if (irValue >= FINGER_IR_THRESHOLD) {

      fingerDetected = true;
      lastFingerSeenMs = millis();

    } else if (fingerDetected &&
               millis() - lastFingerSeenMs > FINGER_LOST_MS) {

      fingerDetected = false;
      resetReadings();
      Serial.println("Finger removed, readings reset.");
    }

    // Ignore samples while there is no finger
    if (!fingerDetected) {
      yield();
      server.handleClient();
      continue;
    }

    // Beat detection runs on every sample (100/second)
    if (checkForBeat(irValue)) {

      const unsigned long now = millis();

      if (lastBeatMs != 0) {
        float bpm = 60000.0f / (now - lastBeatMs);   // time between beats -> BPM

        if (bpm >= 58 && bpm <= 160) {
          updateLiveHeartRate((int)(bpm + 0.5f));
        }
      }

      lastBeatMs = now;
    }

    // The SpO2 algorithm expects 25 samples/second, so only pass on every 4th sample
    static byte skip = 0;
    if (++skip < 4) {
      yield();
      server.handleClient();
      continue;
    }
    skip = 0;

    // -------------------------
    // Fill initial buffer
    // -------------------------
    if (!bufferFilled) {

      redBuffer[sampleIndex] = redValue;
      irBuffer[sampleIndex] = irValue;

      sampleIndex++;

      if (sampleIndex >= SPO2_BUFFER_LENGTH) {

        bufferFilled = true;

        calculateVitals();

        sampleIndex =
          SPO2_BUFFER_LENGTH - SPO2_SHIFT_SAMPLES;

        server.handleClient();
      }
    }

    // -------------------------
    // Continuous operation
    // -------------------------
    else {

      redBuffer[sampleIndex] = redValue;
      irBuffer[sampleIndex] = irValue;

      sampleIndex++;

      if (sampleIndex >= SPO2_BUFFER_LENGTH) {

        shiftBuffers();

        calculateVitals();

        server.handleClient();
      }
    }

    yield();

    server.handleClient();
  }
}

// ============================================================
// Read DHT11
// ============================================================
void updateDHT11() {

  const unsigned long now = millis();

  if (now - lastSlowMs < SLOW_SENSOR_MS) {
    return;
  }

  lastSlowMs = now;

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Check for failed DHT11 reading
  if (!isnan(temperature)) {
    roomTempC = temperature;
  }

  if (!isnan(humidity)) {
    roomHumidity = humidity;
  }

  // Debug information
  if (now - lastDebugMs >= 2000) {

    lastDebugMs = now;

    Serial.printf(
      "Temperature: %.1f C  Humidity: %.1f%%  "
      "SpO2: %d%%  Heart Rate: %d BPM  Finger: %s\n",

      roomTempC,
      roomHumidity,
      spo2Pct,
      heartRateBpm,
      fingerDetected ? "yes" : "no"
    );
  }
}

// ============================================================
// Write one row of the LCD (only if the text changed)
// ============================================================
void lcdShowLine(uint8_t row, const char* text) {

  if (!lcdOk || row >= LCD_ROWS) {
    return;
  }

  // Pad with spaces to 20 characters so old text is erased
  char padded[LCD_COLS + 1];
  snprintf(padded, sizeof(padded), "%-*s", LCD_COLS, text);

  // Skip the write if this row already shows the same text
  if (strcmp(padded, lcdCache[row]) == 0) {
    return;
  }

  strcpy(lcdCache[row], padded);

  lcd.setCursor(0, row);
  lcd.print(padded);
}

// ============================================================
// Update the LCD with the latest readings
// ============================================================
void updateLcd() {

  if (!lcdOk) {
    return;
  }

  const unsigned long now = millis();

  if (now - lastLcdMs < LCD_UPDATE_MS) {
    return;
  }

  lastLcdMs = now;

  char text[32];

  // Row 0: heart rate
  if (!fingerDetected) {
    lcdShowLine(0, "HR: place finger");
  } else if (heartRateBpm > 0) {
    snprintf(text, sizeof(text), "HR: %d BPM", heartRateBpm);
    lcdShowLine(0, text);
  } else {
    lcdShowLine(0, "HR: measuring...");
  }

  // Row 1: SpO2
  if (spo2Pct > 0) {
    snprintf(text, sizeof(text), "SpO2: %d %%", spo2Pct);
    lcdShowLine(1, text);
  } else {
    lcdShowLine(1, "SpO2: --");
  }

  // Row 2: room temperature and humidity
  snprintf(text, sizeof(text), "Room:%.1fC Hum:%.0f%%", roomTempC, roomHumidity);
  lcdShowLine(2, text);

  // Row 3: WiFi status / IP address
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(text, sizeof(text), "IP:%s", WiFi.localIP().toString().c_str());
  } else {
    snprintf(text, sizeof(text), "WiFi: not connected");
  }
  lcdShowLine(3, text);
}

// ============================================================
// Dashboard
// ============================================================
void handleRoot() {

  server.send_P(
    200,
    "text/html",
    DASHBOARD_PAGE
  );
}

// ============================================================
// Vitals API
// ============================================================
void handleVitalsApi() {

  snprintf(
    jsonBuffer,
    sizeof(jsonBuffer),

    "{\"roomT\":%.1f,"
    "\"hum\":%.1f,"
    "\"spo2\":%d,"
    "\"heartRate\":%d,"
    "\"spo2Valid\":%d,"
    "\"finger\":%d,"
    "\"measuring\":%d,"
    "\"max\":%d}",

    roomTempC,
    roomHumidity,
    spo2Pct,
    heartRateBpm,

    (spo2Pct > 0) ? 1 : 0,

    fingerDetected ? 1 : 0,

    (fingerDetected && heartRateBpm < 0) ? 1 : 0,

    maxOk ? 1 : 0
  );

  server.sendHeader(
    "Cache-Control",
    "no-cache"
  );

  server.send(
    200,
    "application/json",
    jsonBuffer
  );
}

// ============================================================
// WiFi setup
// ============================================================
void setupWiFi() {

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.print(
    F("Connecting to WiFi")
  );

  uint8_t attempts = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < 40
  ) {

    delay(500);

    Serial.print('.');

    attempts++;
  }

  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
      F("WiFi failed.")
    );

    return;
  }

  Serial.print(
    F("Open: http://")
  );

  Serial.println(
    WiFi.localIP()
  );
}

// ============================================================
// SETUP
// ============================================================
void setup() {

  Serial.begin(115200);

  delay(500);

  // -------------------------
  // I2C
  // -------------------------
  Wire.begin(
    I2C_SDA_PIN,
    I2C_SCL_PIN
  );

  // -------------------------
  // LCD
  // -------------------------
  if (lcd.begin(LCD_COLS, LCD_ROWS) == 0) {

    lcdOk = true;

    lcd.clear();
    lcdShowLine(0, "Health Monitor");
    lcdShowLine(1, "Starting...");

    Serial.println(F("LCD OK"));

  } else {

    Serial.println(F("LCD not found"));
  }

  // -------------------------
  // DHT11
  // -------------------------
  dht.begin();

  Serial.println(
    F("DHT11 initialized")
  );

  // -------------------------
  // MAX30102
  // -------------------------
  maxOk = particleSensor.begin(
    Wire,
    I2C_SPEED_STANDARD
  );

  if (maxOk) {

    particleSensor.setup(
      30,     // LED brightness
      1,      // sample average
      2,      // LED mode
      100,    // sample rate
      411,    // pulse width
      4096    // ADC range
    );

    particleSensor.setPulseAmplitudeGreen(0);

    Serial.println(
      F("MAX30102 OK")
    );

  } else {

    Serial.println(
      F("MAX30102 not found")
    );
  }

  // -------------------------
  // WiFi
  // -------------------------
  lcdShowLine(1, "Sensors ready");
  lcdShowLine(2, "Connecting WiFi...");
  setupWiFi();
  lcdShowLine(2, "");

  // -------------------------
  // Web server
  // -------------------------
  server.on(
    "/",
    handleRoot
  );

  server.on(
    "/api/vitals",
    handleVitalsApi
  );

  server.begin();

  Serial.println(
    F("Patient Health Monitor ready")
  );
}

// ============================================================
// LOOP
// ============================================================
void loop() {

  server.handleClient();     // harmless when WiFi is offline

  // Sensors and LCD run whether or not WiFi is connected
  updateMax30102();

  updateDHT11();

  updateLcd();

  yield();
}
