// ================================================================
// PRAHARI IIC 3.0 - ESP32 LANDSLIDE MONITORING NODE
// ================================================================

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// ================================================================
// OLED
// ================================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define OLED_RESET -1

// ================================================================
// WIFI
// ================================================================

#define WIFI_SSID "Kshitij's S26 Ultra"
#define WIFI_PASSWORD "12345678"

// ================================================================
// SERVER
// ================================================================

#define SERVER_URL "https://api.prahari.space/data"

// ================================================================
// SENSOR PINS
// ================================================================

#define DHT_PIN 25
#define SOIL_PIN_1 32
#define SOIL_PIN_2 35
#define RAIN_DO_PIN 26

// ================================================================
// SOIL CALIBRATION
// ================================================================

#define S1_DRY_ADC 4095
#define S1_WET_ADC 1000

#define S2_DRY_ADC 4095
#define S2_WET_ADC 1000

// ================================================================
// SETTINGS
// ================================================================

#define DHT_TYPE DHT22

#define ADC_OVERSAMPLE 8

// Data sent to server every 3 seconds
#define SAMPLE_INTERVAL_MS 3000

// Vibration score considered significant
#define VIB_THRESHOLD 2.0f

#define WIFI_RETRY_MAX 10

#define SOIL_DRY 25
#define SOIL_DAMP 50
#define SOIL_WET 80

// ================================================================
// SENSOR OBJECTS
// ================================================================

Adafruit_ADXL345_Unified accel =
  Adafruit_ADXL345_Unified(12345);

DHT dht(DHT_PIN, DHT_TYPE);

Adafruit_BMP085 bmp;

Adafruit_SSD1306 oled(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ================================================================
// GLOBAL VARIABLES
// ================================================================

float prevMag = -1.0f;

unsigned long sampleCount = 0;

// ================================================================
// READ ADC
// ================================================================

int readADCSmoothed(int pin, int samples)
{
  long total = 0;

  for (int i = 0; i < samples; i++)
  {
    total += analogRead(pin);
    delay(2);
  }

  return total / samples;
}

// ================================================================
// SOIL PERCENTAGE
// ================================================================

float toPercent(
  int raw,
  int dryADC,
  int wetADC
)
{
  if (dryADC == wetADC)
    return 0.0f;

  float percentage =
    ((float)(dryADC - raw) /
     (float)(dryADC - wetADC)) * 100.0f;

  return constrain(
    percentage,
    0.0f,
    100.0f
  );
}

// ================================================================
// SOIL STATUS
// ================================================================

const char* classifySoil(float percentage)
{
  if (percentage <= SOIL_DRY)
    return "DRY";

  if (percentage <= SOIL_DAMP)
    return "DAMP";

  if (percentage <= SOIL_WET)
    return "WET";

  return "SAT";
}

// ================================================================
// SOIL GRADIENT
// ================================================================

const char* soilGradient(
  float s1,
  float s2
)
{
  bool topWet = s1 > SOIL_DAMP;
  bool bottomWet = s2 > SOIL_DAMP;

  if (topWet && !bottomWet)
    return "PERCOLATING";

  if (topWet && bottomWet)
    return "SATURATED";

  if (!topWet && bottomWet)
    return "SEEPING";

  return "STABLE";
}

// ================================================================
// OLED LINE
// ================================================================

void oledLine(int y)
{
  oled.drawFastHLine(
    0,
    y,
    SCREEN_WIDTH,
    SSD1306_WHITE
  );
}

// ================================================================
// OLED SPLASH
// ================================================================

void oledSplash()
{
  oled.clearDisplay();

  oled.setTextColor(SSD1306_WHITE);

  oled.setTextSize(2);
  oled.setCursor(4, 8);
  oled.print(F("PRAHARI"));

  oled.setTextSize(1);
  oled.setCursor(8, 36);
  oled.print(F("Landslide Monitoring"));

  oled.setCursor(30, 50);
  oled.print(F("Initializing"));

  oled.display();
}

// ================================================================
// OLED WIFI
// ================================================================

void oledWiFiStatus(
  const char* msg,
  int attempt
)
{
  oled.clearDisplay();

  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);

  oled.setCursor(0, 0);
  oled.print(F("PRAHARI"));

  oledLine(10);

  oled.setCursor(0, 16);
  oled.print(F("WiFi: "));
  oled.print(msg);

  if (attempt > 0)
  {
    oled.setCursor(0, 29);

    oled.print(F("Attempt: "));
    oled.print(attempt);
    oled.print(F("/"));
    oled.print(WIFI_RETRY_MAX);
  }

  oled.display();
}

// ================================================================
// OLED SENSOR READING
// ================================================================

void oledReading(
  float s1Pct,
  float s2Pct,
  float magnitude,
  float vibration,
  int rain
)
{
  oled.clearDisplay();

  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);

  oled.setCursor(0, 0);
  oled.print(F("PRAHARI"));

  oled.setCursor(80, 0);
  oled.print(F("READING"));

  oledLine(9);

  // Soil 1

  oled.setCursor(0, 13);

  oled.print(F("S1:"));
  oled.print((int)s1Pct);
  oled.print(F("%"));

  int bar1 =
    map((int)s1Pct, 0, 100, 0, 60);

  oled.drawRect(
    44,
    13,
    62,
    6,
    SSD1306_WHITE
  );

  oled.fillRect(
    44,
    13,
    bar1,
    6,
    SSD1306_WHITE
  );

  // Soil 2

  oled.setCursor(0, 23);

  oled.print(F("S2:"));
  oled.print((int)s2Pct);
  oled.print(F("%"));

  int bar2 =
    map((int)s2Pct, 0, 100, 0, 60);

  oled.drawRect(
    44,
    23,
    62,
    6,
    SSD1306_WHITE
  );

  oled.fillRect(
    44,
    23,
    bar2,
    6,
    SSD1306_WHITE
  );

  oledLine(33);

  // Acceleration

  oled.setCursor(0, 37);

  oled.print(F("|a|:"));
  oled.print(magnitude, 2);
  oled.print(F("m/s2"));

  // Vibration 0-10

  oled.setCursor(0, 48);

  oled.print(F("VIB:"));
  oled.print(vibration, 1);
  oled.print(F("/10"));

  // Rain

  oled.setCursor(70, 48);

  oled.print(F("RAIN:"));
  oled.print(
    rain ? F("YES") : F("NO")
  );

  oled.setCursor(0, 57);
  oled.print(F("Sending..."));

  oled.display();
}

// ================================================================
// OLED RISK
// ================================================================

void oledRisk(
  const char* risk,
  float confidence,
  int sampleId,
  float s1Pct,
  float s2Pct,
  float vibration
)
{
  bool isHigh =
    strcmp(risk, "HIGH") == 0;

  bool isMedium =
    strcmp(risk, "MEDIUM") == 0;

  oled.clearDisplay();

  // --------------------------------------------------------------
  // HIGH
  // --------------------------------------------------------------

  if (isHigh)
  {
    oled.fillRect(
      0,
      0,
      SCREEN_WIDTH,
      SCREEN_HEIGHT,
      SSD1306_WHITE
    );

    oled.setTextColor(
      SSD1306_BLACK
    );

    oled.setTextSize(1);

    oled.setCursor(2, 2);
    oled.print(F("! PRAHARI ALERT !"));

    oled.setTextSize(3);

    oled.setCursor(8, 16);
    oled.print(F("HIGH"));

    oled.setTextSize(1);

    oled.setCursor(2, 44);
    oled.print(F("RISK: HIGH"));

    oled.setCursor(80, 44);
    oled.print(F("C:"));
    oled.print((int)confidence);
    oled.print(F("%"));

    oled.setCursor(2, 56);
    oled.print(F("S1:"));
    oled.print((int)s1Pct);
    oled.print(F("%"));

    oled.setCursor(48, 56);
    oled.print(F("S2:"));
    oled.print((int)s2Pct);
    oled.print(F("%"));

    oled.setCursor(94, 56);

    if (vibration >= VIB_THRESHOLD)
      oled.print(F("VIB!"));
  }

  // --------------------------------------------------------------
  // NORMAL / MEDIUM
  // --------------------------------------------------------------

  else
  {
    oled.setTextColor(
      SSD1306_WHITE
    );

    oled.setTextSize(1);

    oled.setCursor(0, 0);
    oled.print(F("PRAHARI"));

    oled.setCursor(74, 0);
    oled.print(F("#"));
    oled.print(sampleId);

    oledLine(9);

    oled.setTextSize(2);

    if (isMedium)
      oled.setCursor(4, 14);
    else
      oled.setCursor(22, 14);

    oled.print(risk);

    oled.setTextSize(1);

    oled.setCursor(0, 34);

    oled.print(F("Conf:"));
    oled.print((int)confidence);
    oled.print(F("%"));

    oledLine(43);

    oled.setCursor(0, 47);

    oled.print(F("S1:"));
    oled.print((int)s1Pct);
    oled.print(F("%"));

    oled.setCursor(52, 47);

    oled.print(F("S2:"));
    oled.print((int)s2Pct);
    oled.print(F("%"));

    oled.setCursor(0, 57);

    oled.print(F("VIB:"));
    oled.print(vibration, 1);
    oled.print(F("/10"));

    oled.setCursor(91, 57);
    oled.print(F("IIC3"));
  }

  oled.display();
}

// ================================================================
// OLED ERROR
// ================================================================

void oledError(const char* msg)
{
  oled.clearDisplay();

  oled.setTextColor(
    SSD1306_WHITE
  );

  oled.setTextSize(1);

  oled.setCursor(0, 0);
  oled.print(F("PRAHARI - ERROR"));

  oledLine(10);

  oled.setCursor(0, 20);
  oled.print(msg);

  oled.display();
}

// ================================================================
// WIFI CONNECT
// ================================================================

void connectWiFi()
{
  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  int attempts = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < WIFI_RETRY_MAX
  )
  {
    attempts++;

    oledWiFiStatus(
      "Connecting...",
      attempts
    );

    Serial.print(".");

    delay(500);
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println(
      F("WiFi connected")
    );

    Serial.print(F("IP: "));
    Serial.println(
      WiFi.localIP()
    );

    oledWiFiStatus(
      "Connected!",
      0
    );

    delay(1000);
  }
  else
  {
    Serial.println(
      F("WiFi FAILED")
    );

    oledError(
      "WiFi failed!"
    );

    delay(2000);
  }
}

// ================================================================
// SETUP
// ================================================================

void setup()
{
  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println(
    F("==============================")
  );

  Serial.println(
    F(" PRAHARI IIC 3.0")
  );

  Serial.println(
    F("==============================")
  );

  // Rain sensor

  pinMode(
    RAIN_DO_PIN,
    INPUT
  );

  // ADC

  analogReadResolution(12);

  analogSetAttenuation(
    ADC_11db
  );

  // I2C

  Wire.begin(
    21,
    22
  );

  Wire.setClock(100000);

  // OLED

  if (!oled.begin(
    SSD1306_SWITCHCAPVCC,
    OLED_ADDR
  ))
  {
    Serial.println(
      F("OLED not found")
    );

    while (true)
      delay(1000);
  }

  oledSplash();

  delay(1500);

  // ADXL345

  if (!accel.begin())
  {
    Serial.println(
      F("ADXL345 not found")
    );

    oledError(
      "ADXL345 not found!"
    );

    while (true)
      delay(1000);
  }

  accel.setRange(
    ADXL345_RANGE_2_G
  );

  Serial.println(
    F("ADXL345 OK")
  );

  // DHT22

  dht.begin();

  Serial.println(
    F("DHT22 OK")
  );

  // BMP180

  if (!bmp.begin())
  {
    Serial.println(
      F("BMP180 not found")
    );

    oledError(
      "BMP180 not found!"
    );

    while (true)
      delay(1000);
  }

  Serial.println(
    F("BMP180 OK")
  );

  Serial.println(
    F("All sensors OK")
  );

  connectWiFi();
}

// ================================================================
// LOOP
// ================================================================

void loop()
{
  // ==============================================================
  // WIFI
  // ==============================================================

  if (WiFi.status() != WL_CONNECTED)
  {
    oledWiFiStatus(
      "Reconnecting...",
      0
    );

    connectWiFi();
  }

  sampleCount++;

  // ==============================================================
  // ADXL345
  // ==============================================================

  sensors_event_t event;

  accel.getEvent(&event);

  float ax =
    event.acceleration.x;

  float ay =
    event.acceleration.y;

  float az =
    event.acceleration.z;

  // Total acceleration

  float magnitude =
    sqrt(
      ax * ax +
      ay * ay +
      az * az
    );

  // ==============================================================
  // TILT
  // ==============================================================

  float pitch =
    atan2(
      -ax,
      sqrt(
        ay * ay +
        az * az
      )
    ) *
    180.0f / PI;

  float roll =
    atan2(
      ay,
      az
    ) *
    180.0f / PI;

  // ==============================================================
  // VIBRATION 0-10
  // ==============================================================

  float vibration = 0.0f;

  if (prevMag >= 0)
  {
    float accelerationChange =
      fabs(
        magnitude - prevMag
      );

    // Convert acceleration change
    // to a simple 0-10 scale

    vibration =
      accelerationChange / 0.5f;

    vibration =
      constrain(
        vibration,
        0.0f,
        10.0f
      );
  }

  prevMag = magnitude;

  // ==============================================================
  // SOIL
  // ==============================================================

  int s1Raw =
    readADCSmoothed(
      SOIL_PIN_1,
      ADC_OVERSAMPLE
    );

  int s2Raw =
    readADCSmoothed(
      SOIL_PIN_2,
      ADC_OVERSAMPLE
    );

  float s1Pct =
    toPercent(
      s1Raw,
      S1_DRY_ADC,
      S1_WET_ADC
    );

  float s2Pct =
    toPercent(
      s2Raw,
      S2_DRY_ADC,
      S2_WET_ADC
    );

  // ==============================================================
  // DHT22
  // ==============================================================

  float dhtTemp =
    dht.readTemperature();

  float dhtHum =
    dht.readHumidity();

  bool dhtOK =
    !isnan(dhtTemp) &&
    !isnan(dhtHum);

  // ==============================================================
  // BMP180
  // ==============================================================

  float bmpTemp =
    bmp.readTemperature();

  float bmpPressure =
    bmp.readPressure() /
    100.0f;

  // ==============================================================
  // RAIN
  // ==============================================================

  int rainDetected =
    digitalRead(
      RAIN_DO_PIN
    ) == LOW ? 1 : 0;

  // ==============================================================
  // SERIAL OUTPUT
  // ==============================================================

  Serial.println();
  Serial.println(
    F("------------------------------")
  );

  Serial.print(F("Sample: "));
  Serial.println(sampleCount);

  Serial.print(F("S1: "));
  Serial.print(s1Pct, 1);
  Serial.println(F("%"));

  Serial.print(F("S2: "));
  Serial.print(s2Pct, 1);
  Serial.println(F("%"));

  Serial.print(F("Acceleration: "));
  Serial.print(magnitude, 2);
  Serial.println(F(" m/s2"));

  Serial.print(
    F("Vibration: ")
  );

  Serial.print(
    vibration,
    1
  );

  Serial.println(
    F(" / 10")
  );

  Serial.print(F("Rain: "));
  Serial.println(
    rainDetected ? "YES" : "NO"
  );

  if (dhtOK)
  {
    Serial.print(F("Temperature: "));
    Serial.print(dhtTemp, 1);
    Serial.println(F(" C"));

    Serial.print(F("Humidity: "));
    Serial.print(dhtHum, 1);
    Serial.println(F(" %"));
  }
  else
  {
    Serial.println(
      F("DHT read failed")
    );
  }

  Serial.print(F("Pressure: "));
  Serial.print(
    bmpPressure,
    1
  );

  Serial.println(
    F(" hPa")
  );

  Serial.print(F("Pitch: "));
  Serial.println(
    pitch,
    1
  );

  Serial.print(F("Roll: "));
  Serial.println(
    roll,
    1
  );

  // ==============================================================
  // OLED
  // ==============================================================

  oledReading(
    s1Pct,
    s2Pct,
    magnitude,
    vibration,
    rainDetected
  );

  // ==============================================================
  // JSON
  // ==============================================================

  StaticJsonDocument<1024> doc;

  doc["ax"] = ax;
  doc["ay"] = ay;
  doc["az"] = az;

  doc["magnitude"] =
    magnitude;

  doc["pitch"] =
    pitch;

  doc["roll"] =
    roll;

  // 0-10 vibration scale

  doc["vibration"] =
    vibration;

  doc["s1_adc"] =
    s1Raw;

  doc["s2_adc"] =
    s2Raw;

  doc["s1_pct"] =
    s1Pct;

  doc["s2_pct"] =
    s2Pct;

  doc["s1_status"] =
    classifySoil(s1Pct);

  doc["s2_status"] =
    classifySoil(s2Pct);

  doc["soil_gradient"] =
    soilGradient(
      s1Pct,
      s2Pct
    );

  if (dhtOK)
  {
    doc["dht_temp"] =
      dhtTemp;

    doc["dht_humidity"] =
      dhtHum;
  }
  else
  {
    doc["dht_temp"] =
      -999;

    doc["dht_humidity"] =
      -999;
  }

  doc["bmp_temp"] =
    bmpTemp;

  doc["bmp_pressure"] =
    bmpPressure;

  doc["rain_detected"] =
    rainDetected;

  doc["sample_id"] =
    sampleCount;

  String payload;

  serializeJson(
    doc,
    payload
  );

  Serial.print(
    F("POST: ")
  );

  Serial.println(payload);

  // ==============================================================
  // SEND TO BACKEND
  // ==============================================================

  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClientSecure client;

    // HTTPS prototype
    client.setInsecure();

    HTTPClient http;

    http.setTimeout(10000);

    if (
      http.begin(
        client,
        SERVER_URL
      )
    )
    {
      http.addHeader(
        "Content-Type",
        "application/json"
      );

      int httpCode =
        http.POST(payload);

      Serial.print(
        F("HTTP Code: ")
      );

      Serial.println(
        httpCode
      );

      if (httpCode > 0)
      {
        String response =
          http.getString();

        Serial.print(
          F("SERVER: ")
        );

        Serial.println(
          response
        );

        if (httpCode == 200)
        {
          StaticJsonDocument<512>
            responseDoc;

          DeserializationError error =
            deserializeJson(
              responseDoc,
              response
            );

          if (!error)
          {
            const char* risk =
              responseDoc["risk"]
              | "LOW";

            float confidence =
              responseDoc["confidence"]
              | 0.0f;

            int sampleId =
              responseDoc["sample_id"]
              | (int)sampleCount;

            oledRisk(
              risk,
              confidence,
              sampleId,
              s1Pct,
              s2Pct,
              vibration
            );

            Serial.print(
              F("RISK: ")
            );

            Serial.println(
              risk
            );

            Serial.print(
              F("CONFIDENCE: ")
            );

            Serial.print(
              confidence
            );

            Serial.println(
              F("%")
            );
          }
          else
          {
            Serial.println(
              F("Invalid server JSON")
            );

            oledError(
              "Invalid server data"
            );
          }
        }
        else
        {
          Serial.println(
            F("Server error")
          );

          oledError(
            "Server error!"
          );
        }
      }
      else
      {
        Serial.print(
          F("POST failed: ")
        );

        Serial.println(
          http.errorToString(
            httpCode
          )
        );

        oledError(
          "POST failed!"
        );
      }

      http.end();
    }
    else
    {
      Serial.println(
        F("HTTPS begin failed")
      );

      oledError(
        "HTTPS failed!"
      );
    }
  }
  else
  {
    Serial.println(
      F("WiFi disconnected")
    );

    oledError(
      "WiFi disconnected!"
    );
  }

  // ==============================================================
  // WAIT
  // ==============================================================

  delay(
    SAMPLE_INTERVAL_MS
  );
}