#define BLYNK_TEMPLATE_ID "TMPL3eAkm2bH9"
#define BLYNK_TEMPLATE_NAME "smart helmet"
#define BLYNK_AUTH_TOKEN "reQqpL2bUBJdeHIPTo1bQpGjl_akVYBO"
#define BLYNK_DEVICE_NAME "smart helmet"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// === WiFi Credentials ===
char auth[] = "reQqpL2bUBJdeHIPTo1bQpGjl_akVYBO";
char ssid[] = "Shinchan";
char pass[] = "qwertyui";

// === LCD Setup ===
LiquidCrystal_I2C lcd(0x27, 16, 2);

// === Helmet Detection ===
const int irSensorPin = 32;
const int ledPin = 25;

// === Alcohol Detection ===
const int mq3AnalogPin = 33;
const int buzzerPin = 26;

// === Relay + Motor ===
const int relayPin = 27;

// === Eye Blink Sensor ===
const int eyeBlinkPin = 4;
unsigned long closeStartTime = 0;
bool timerStarted = false;

// === Alcohol calibration ===
int baseline = 0;
const int samples = 100;
const int detectionOffset = 100;

// === MPU6050 ===
const int MPU_ADDR = 0x68;
const float TILT_THRESHOLD = 0.7;

// === Flags ===
bool helmetOn = false;
bool alcoholDetected = false;
bool accidentDetected = false;

void setup() {

  Serial.begin(115200);

  Wire.begin(21, 22);
  Wire.setClock(100000);

  lcd.init();
  lcd.backlight();

  pinMode(irSensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(mq3AnalogPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(eyeBlinkPin, INPUT);

  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(relayPin, LOW);

  // 🔹 Blynk Start
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Startup message
  lcd.setCursor(3, 0);
  lcd.print("BIKE ON");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Please Wear");
  lcd.setCursor(0, 1);
  lcd.print("Helmet");

  // MQ3 Calibration
  Serial.println("Calibrating MQ3...");
  long total = 0;
  int minVal = 4095;

  for (int i = 0; i < samples; i++) {
    int val = analogRead(mq3AnalogPin);
    total += val;
    if (val < minVal) minVal = val;
    delay(50);
  }

  baseline = (total / samples + minVal) / 2;

  Serial.print("Baseline = ");
  Serial.println(baseline);

  // MPU6050 start
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println("MPU6050 Ready");
}

void loop() {

  Blynk.run();  // 🔹 IMPORTANT

  int irState = digitalRead(irSensorPin);

  if (irState == HIGH) {

    helmetOn = false;

    digitalWrite(ledPin, HIGH);
    digitalWrite(buzzerPin, LOW);
    digitalWrite(relayPin, LOW);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Please Wear");
    lcd.setCursor(0, 1);
    lcd.print("Helmet");

    Serial.println("Helmet Not Detected");

    // 🔹 Blynk
    Blynk.virtualWrite(V0, 0);
  } 
  else {

    helmetOn = true;
    digitalWrite(ledPin, LOW);

    // 🔹 Blynk
    Blynk.virtualWrite(V0, 1);

    int sensorValue = analogRead(mq3AnalogPin);

    Serial.print("Alcohol Sensor: ");
    Serial.println(sensorValue);

    // === Alcohol Detection ===
    if (sensorValue > baseline + detectionOffset) {

      alcoholDetected = true;

      digitalWrite(buzzerPin, HIGH);
      digitalWrite(relayPin, LOW);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ALCOHOL");
      lcd.setCursor(0, 1);
      lcd.print("DETECTED!");

      Serial.println("Alcohol Detected");

      // 🔹 Blynk
      Blynk.virtualWrite(V1, 1);

    } 
    else {

      alcoholDetected = false;

      // 🔹 Blynk
      Blynk.virtualWrite(V1, 0);

      // === DROWSINESS DETECTION ===
      int eyeState = digitalRead(eyeBlinkPin);
      Serial.print("Eye State: ");
      Serial.println(eyeState);

      if (eyeState == HIGH) {

        if (!timerStarted) {
          closeStartTime = millis();
          timerStarted = true;
        }

        if (millis() - closeStartTime >= 3000) {

          digitalWrite(buzzerPin, HIGH);
          digitalWrite(relayPin, LOW);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("DROWSINESS");
          lcd.setCursor(0, 1);
          lcd.print("DETECTED!");

          Serial.println("Drowsiness Detected");

          // 🔹 Blynk
          Blynk.virtualWrite(V2, 1);

          delay(500);
          return;
        }

      } 
      else {

        timerStarted = false;
        digitalWrite(buzzerPin, LOW);

        // 🔹 Blynk
        Blynk.virtualWrite(V2, 0);
      }

      // === ACCIDENT DETECTION ===
      accidentDetected = checkAccident();

      if (accidentDetected) {

        digitalWrite(buzzerPin, HIGH);
        digitalWrite(relayPin, LOW);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ACCIDENT");
        lcd.setCursor(0, 1);
        lcd.print("DETECTED!");

        Serial.println("Accident Detected");

        // 🔹 Blynk
        Blynk.virtualWrite(V3, 1);

      } 
      else {

        digitalWrite(buzzerPin, LOW);
        digitalWrite(relayPin, HIGH);

        lcd.setCursor(0, 0);
        lcd.print("Helmet Worn   ");
        lcd.setCursor(0, 1);
        lcd.print("Safe to Ride  ");

        Serial.println("Safe to Ride");

        // 🔹 Blynk
        Blynk.virtualWrite(V3, 0);
      }
    }
  }

  delay(500);
}


// === Accident Detection ===
bool checkAccident() {

  int16_t AcX, AcY, AcZ;

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);

  Wire.requestFrom(MPU_ADDR, 6, true);

  if (Wire.available() < 6) return false;

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  float Ax = AcX / 16384.0;
  float Ay = AcY / 16384.0;
  float Az = AcZ / 16384.0;

  Serial.print("Ax: "); Serial.print(Ax);
  Serial.print(" Ay: "); Serial.print(Ay);
  Serial.print(" Az: "); Serial.println(Az);

  if (abs(Ax) > TILT_THRESHOLD || abs(Ay) > TILT_THRESHOLD) {
    return true;
  }

  return false;
}