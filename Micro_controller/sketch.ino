#include "DHTesp.h"
#include "RTClib.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

String GAS_URL = "https://script.google.com/macros/s/KEY/exec";

const int Gas_analog = 27;
const int DHT_PIN = 19;
const int Buzzer_pin_temp = 26;
const int Buzzer_pin_pot = 25;
const int Pot_pin = 34;
int tempDigit;
int potDigit;

// Top LEDs
const int Gt1 = 23;
const int Gt2 = 18;
const int Yt1 = 5;
const int Rt1 = 17;
const int Rt2 = 16;

// Bottom LEDs
const int Gb1 = 4;
const int Gb2 = 0;
const int Yb1 = 2;
const int Rb1 = 33;
const int Rb2 = 32;

int gasval;
int potval;
float temperatureValue, humidityValue;

unsigned long lastLedUpdate = 0;
const unsigned long ledUpdateInterval = 50;

unsigned long lastSensorRead = 0;
const unsigned long sensorInterval = 5000;

unsigned long lastPost = 0;
const unsigned long postInterval = 5000;

RTC_DS1307 rtc;
DHTesp dhtSensor;

char daysOfTheWeek[7][12] = {
  "Sunday", "Monday", "Tuesday", "Wednesday",
  "Thursday", "Friday", "Saturday"
};

void setup() {
  Serial.begin(115200);
   WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  pinMode(Pot_pin, INPUT);
  pinMode(Buzzer_pin_pot, OUTPUT);
  pinMode(Buzzer_pin_temp, OUTPUT);
  pinMode(Gas_analog, INPUT);
  // Top LEDs
  pinMode(Gt1, OUTPUT);
  pinMode(Gt2, OUTPUT);
  pinMode(Yt1, OUTPUT);
  pinMode(Rt1, OUTPUT);
  pinMode(Rt2, OUTPUT);

  // Bottom LEDs
  pinMode(Gb1, OUTPUT);
  pinMode(Gb2, OUTPUT);
  pinMode(Yb1, OUTPUT);
  pinMode(Rb1, OUTPUT);
  pinMode(Rb2, OUTPUT);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  Serial.println("MQ2 warming up");

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  delay(200);
}

void loop() {
  unsigned long currentMillis = millis();

  // ======== SENSOR READS EVERY 2s ========
  if (currentMillis - lastSensorRead >= sensorInterval) {
    lastSensorRead = currentMillis;

    // Read DHT22 once
    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    temperatureValue = data.temperature;
    humidityValue = data.humidity;

    gasval = analogRead(Gas_analog);
    DateTime now = rtc.now();

    Serial.println("Temp: " + String(temperatureValue, 2) + "°C");
    Serial.println("Humidity: " + String(humidityValue, 1) + "%");
    Serial.println("---");

    Serial.println(gasval);
    if (gasval <= 3000) {
      Serial.println("Smoke: -");
    } else {
      Serial.println("Smoke: Detected!");
    }

    Serial.print("Current time: ");
    Serial.print(now.year(), DEC); Serial.print('/');
    Serial.print(now.month(), DEC); Serial.print('/');
    Serial.print(now.day(), DEC); Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC); Serial.print(':');
    Serial.print(now.minute(), DEC); Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println("---");
    Serial.print("Pot: "); Serial.print(potval);
    Serial.print(" -> Digit: "); Serial.println(potDigit);
    Serial.print("Temp: "); Serial.print(temperatureValue);
    Serial.print(" -> Digit: "); Serial.println(tempDigit);
  }

  // ======== LED UPDATES EVERY 50ms ========
  if (currentMillis - lastLedUpdate >= ledUpdateInterval) {
    lastLedUpdate = currentMillis;
    potval = analogRead(Pot_pin);
    
    if (temperatureValue < -10) tempDigit = 1;
    else if (temperatureValue < 10) tempDigit = 2;
    else if (temperatureValue < 30) tempDigit = 3;
    else if (temperatureValue < 50) tempDigit = 4;
    else tempDigit = 5; // 50°C and above

    // ---- Map Potentiometer to Digits (Range-based) ----
    if (potval < 820) potDigit = 1;         // 0   -  819
    else if (potval < 1640) potDigit = 2;   // 820 - 1639
    else if (potval < 2460) potDigit = 3;   // 1640- 2459
    else if (potval < 3280) potDigit = 4;   // 2460- 3279
    else potDigit = 5;                      // 3280 and above

    // Turn off all LEDs and buzzer
    digitalWrite(Gt1, LOW); digitalWrite(Gt2, LOW);
    digitalWrite(Yt1, LOW); digitalWrite(Rt1, LOW); digitalWrite(Rt2, LOW);
    digitalWrite(Gb1, LOW); digitalWrite(Gb2, LOW);
    digitalWrite(Yb1, LOW); digitalWrite(Rb1, LOW); digitalWrite(Rb2, LOW);
    digitalWrite(Buzzer_pin_pot, LOW); digitalWrite(Buzzer_pin_temp , LOW);

    // Bottom LEDs (Temperature)
    if (tempDigit == 1) digitalWrite(Gb1, HIGH);
    else if (tempDigit == 2) digitalWrite(Gb2, HIGH);
    else if (tempDigit == 3) digitalWrite(Yb1, HIGH);
    else if (tempDigit == 4) digitalWrite(Rb1, HIGH);
    else if (tempDigit == 5) {
      digitalWrite(Rb2, HIGH);
      digitalWrite(Buzzer_pin_temp, HIGH);
    }

    // Top LEDs (Potentiometer)
    if (potDigit == 1) digitalWrite(Gt1, HIGH);
    else if (potDigit == 2) digitalWrite(Gt2, HIGH);
    else if (potDigit == 3) digitalWrite(Yt1, HIGH);
    else if (potDigit == 4) digitalWrite(Rt1, HIGH);
    else if (potDigit == 5) {
      digitalWrite(Rt2, HIGH);
      digitalWrite(Buzzer_pin_pot, HIGH);
    }
  }

  if (WiFi.status() == WL_CONNECTED && currentMillis - lastPost >= postInterval) {
    lastPost = currentMillis;
    HTTPClient http;

    http.begin(GAS_URL);
    http.addHeader("Content-Type", "application/json");
    DateTime now = rtc.now();
    String currentTime = String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) + " " +
    String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());

   String jsonData = "{";
   jsonData += "\"temperature\": " + String(temperatureValue, 2) + ", ";
   jsonData += "\"humidity\": " + String(humidityValue, 1) + ", ";
   jsonData += "\"potval\": " + String(potval) + ", ";
   jsonData += "\"gasval\": " + String(gasval) + ", ";
   jsonData += "\"time\": \"" + String(currentTime) + "\", ";
   jsonData += "\"potdigit\": " + String(potDigit) + ", ";
   jsonData += "\"tempdigit\": " + String(tempDigit);
   jsonData += "}";

    int httpResponseCode = http.POST(jsonData);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    http.end();
  }
} 



