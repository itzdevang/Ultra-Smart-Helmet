#include <Servo.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(2, 3, 4, 5, 6, 7);
Servo myServo;

int ultrasonicPin = 12;
int LDR_PIN = A0;
int LED_PIN = 11;
int threshold = 500;
int motorpin = 13;
int pirPin = A1;
int tempPin = A2;
int redPin = 8;
int greenPin = 9;
int servoPin = 10;

float temperatureC;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(motorpin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(ultrasonicPin, OUTPUT);
  pinMode(pirPin, INPUT);

  lcd.begin(16, 2);
  Serial.begin(9600);

  myServo.attach(servoPin);
}

void loop() {
  // ===== 1. Ultrasonic Sensor =====
  long duration;
  float distance;

  pinMode(ultrasonicPin, OUTPUT);
  digitalWrite(ultrasonicPin, LOW);
  delayMicroseconds(2);
  digitalWrite(ultrasonicPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(ultrasonicPin, LOW);

  pinMode(ultrasonicPin, INPUT);
  duration = pulseIn(ultrasonicPin, HIGH, 30000);
  distance = duration * 0.0343 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 0 && distance <= 50) {
    myServo.write(90);
  } else {
    myServo.write(0);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Distance:");
  lcd.setCursor(0, 1);
  lcd.print(distance);
  lcd.print(" cm");
  delay(1500);

  // ===== 2. PIR Sensor =====
  int pirState = digitalRead(pirPin);

  if (pirState == HIGH) {
    digitalWrite(motorpin, HIGH);
    Serial.println("Motion Detected");
  } else {
    digitalWrite(motorpin, LOW);
    Serial.println("No Motion");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PIR Motion:");
  lcd.setCursor(0, 1);
  lcd.print(pirState == HIGH ? "Motion Detected" : "No Motion");
  delay(1500);

  // ===== 3. LDR Sensor =====
  int lightLevel = analogRead(LDR_PIN);
  Serial.print("Light Level: ");
  Serial.println(lightLevel);

  if (lightLevel < threshold) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Light Level:");
  lcd.print(lightLevel, 1);
  lcd.setCursor(0, 1);
  lcd.print(lightLevel < threshold ? "Dark" : "Bright");
  delay(1500);

  // ===== 4. Temperature Sensor =====
  int sensorValue = analogRead(tempPin);
  temperatureC = (sensorValue * 5.0 * 100.0) / 1024.0;

  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.write(176);
  Serial.println("C");

  if (temperatureC > 40.0) {
    digitalWrite(redPin, HIGH);
    digitalWrite(greenPin, LOW);
  } else {
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, HIGH);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperatureC, 1);
  lcd.write(176); // degree symbol
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print(temperatureC > 40.0 ? "HOT!" : "Normal");
  delay(1500);
}
