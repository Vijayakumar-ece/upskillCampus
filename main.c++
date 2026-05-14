  #include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid       = "YOUR_WIFI_SSID";
const char* password   = "YOUR_WIFI_PASSWORD";
const char* mqttServer = "broker.hivemq.com";   // Free public broker
const int   mqttPort   = 1883;
const char* mqttTopic  = "iot/safety/data";
const char* alertTopic = "iot/safety/alert";

#define MQ2_AOUT    34
#define MQ2_DOUT    35
#define MQ7_AOUT    32
#define MQ7_DOUT    33
#define DHT_PIN      4
#define FLAME_DOUT  14
#define FLAME_AOUT  36
#define VIBRATION   27
#define PIR_PIN     26
#define BUZZER      25
#define LED_RED     18
#define LED_YELLOW  19
#define LED_GREEN   21

#define DHT_TYPE     DHT11
#define SCREEN_W     128
#define SCREEN_H      64
#define OLED_ADDR   0x3C

#define GAS_THRESHOLD      2000   // ADC raw (0–4095)
#define CO_THRESHOLD       1800
#define TEMP_THRESHOLD       55   // °C
#define HUMIDITY_THRESHOLD   85   // %RH

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 oled(SCREEN_W, SCREEN_H, &Wire, -1);
WiFiClient espClient;
PubSubClient mqtt(espClient);

unsigned long lastPublish = 0;
const long    publishInterval = 5000;   // 5 seconds
bool alertActive = false;

void connectWiFi();
void connectMQTT();
void readSensors(int&, int&, float&, float&, bool&, bool&, bool&);
void triggerAlert(String reason);
void clearAlert();
void publishData(int, int, float, float, bool, bool, bool);
void updateOLED(int, int, float, float, bool);

void setup() {
  Serial.begin(115200);

  // Pin modes
  pinMode(MQ2_DOUT,   INPUT);
  pinMode(MQ7_DOUT,   INPUT);
  pinMode(FLAME_DOUT, INPUT_PULLUP);
  pinMode(VIBRATION,  INPUT);
  pinMode(PIR_PIN,    INPUT);
  pinMode(BUZZER,     OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN,  OUTPUT);

  digitalWrite(LED_GREEN, HIGH);

  dht.begin();
  Wire.begin();
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[ERROR] OLED init failed");
  }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(10, 28);
  oled.print("Safety System v1.0");
  oled.display();
  delay(2000);
  connectWiFi();
  mqtt.setServer(mqttServer, mqttPort);

  Serial.println("[READY] System initialised.");
}

void loop() {
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  int   gasLevel, coLevel;
  float temperature, humidity;
  bool  flameDetected, vibrationDetected, motionDetected;

  readSensors(gasLevel, coLevel, temperature, humidity,
              flameDetected, vibrationDetected, motionDetected);

  alertActive = false;
  String hazardReason = "";

  if (gasLevel > GAS_THRESHOLD) {
    hazardReason += "GAS_LEAK ";
    alertActive = true;
  }
  if (coLevel > CO_THRESHOLD) {
    hazardReason += "CO_HAZARD ";
    alertActive = true;
  }
  if (temperature > TEMP_THRESHOLD) {
    hazardReason += "HIGH_TEMP ";
    alertActive = true;
  }
  if (flameDetected) {
    hazardReason += "FLAME ";
    alertActive = true;
  }
  if (vibrationDetected) {
    hazardReason += "VIBRATION ";
    alertActive = true;
  }

  if (alertActive) {
    triggerAlert(hazardReason);
  } else {
    clearAlert();
  }

  if (gasLevel > GAS_THRESHOLD * 0.7 || temperature > TEMP_THRESHOLD * 0.9) {
    digitalWrite(LED_YELLOW, HIGH);
  } else {
    digitalWrite(LED_YELLOW, LOW);
  }

  if (motionDetected) {
    Serial.println("[INFO] Motion detected in restricted zone.");
  }

  updateOLED(gasLevel, coLevel, temperature, humidity, alertActive);

  if (millis() - lastPublish > publishInterval) {
    publishData(gasLevel, coLevel, temperature, humidity,
                flameDetected, vibrationDetected, motionDetected);
    lastPublish = millis();
  }

  delay(500);
}

void readSensors(int& gas, int& co, float& temp, float& hum,
                 bool& flame, bool& vibr, bool& motion) {
  gas   = analogRead(MQ2_AOUT);
  co    = analogRead(MQ7_AOUT);
  temp  = dht.readTemperature();
  hum   = dht.readHumidity();

  if (isnan(temp)) temp = -1.0;
  if (isnan(hum))  hum  = -1.0;

  flame  = (digitalRead(FLAME_DOUT) == LOW);  
  vibr   = (digitalRead(VIBRATION)  == HIGH);
  motion = (digitalRead(PIR_PIN)    == HIGH);

  Serial.printf("[SENSOR] Gas:%d CO:%d Temp:%.1f°C Hum:%.0f%% "
                "Flame:%d Vibr:%d Motion:%d\n",
                gas, co, temp, hum, flame, vibr, motion);
}
void triggerAlert(String reason) {
  Serial.println("[ALERT] HAZARD DETECTED: " + reason);
  digitalWrite(LED_RED,   HIGH);
  digitalWrite(LED_GREEN, LOW);
  for (int i = 0; i < 3; i++)
   {
    digitalWrite(BUZZER, HIGH); delay(200);
    digitalWrite(BUZZER, LOW);  delay(100);
  }
  String alertMsg = "{\"alert\":true,\"reason\":\"" + reason + "\"}";
  mqtt.publish(alertTopic, alertMsg.c_str());
}

void clearAlert() {
  digitalWrite(LED_RED,   LOW);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(BUZZER,    LOW);
}

void publishData(int gas, int co, float temp, float hum,
                 bool flame, bool vibr, bool motion) {
  char payload[256];
  snprintf(payload, sizeof(payload),
    "{\"gas\":%d,\"co\":%d,\"temp\":%.1f,"
    "\"hum\":%.0f,\"flame\":%d,\"vibr\":%d,\"motion\":%d}",
    gas, co, temp, hum, flame, vibr, motion);

  mqtt.publish(mqttTopic, payload);
  Serial.println("[MQTT] Published: " + String(payload));
}

void updateOLED(int gas, int co, float temp, float hum, bool alert) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  oled.setCursor(0, 0);
  oled.print(alert ? "!! HAZARD ALERT !!" : "  System: NORMAL  ");

  oled.setCursor(0, 14);
  oled.printf("Gas: %4d  CO: %4d", gas, co);

  oled.setCursor(0, 27);
  oled.printf("Temp: %.1f C  Hum: %.0f%%", temp, hum);

  oled.setCursor(0, 40);
  oled.printf("Flame:%s Vibr:%s",
    (digitalRead(FLAME_DOUT) == LOW) ? "YES" : " NO",
    (digitalRead(VIBRATION)  == HIGH) ? "YES" : " NO");

  oled.setCursor(0, 53);
  oled.print("WiFi: ");
  oled.print(WiFi.status() == WL_CONNECTED ? "OK " : "OFF");
  oled.print(" MQTT: ");
  oled.print(mqtt.connected() ? "OK" : "OFF");

  oled.display();
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("[WiFi] Connecting");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500); Serial.print("."); attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[WiFi] Failed — running in offline mode.");
  }
}

void connectMQTT() {
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    String clientId = "ESP32Safety_" + String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("[MQTT] Connected.");
      mqtt.subscribe(alertTopic);
    } else {
      Serial.printf("[MQTT] Failed (rc=%d), retrying...\n", mqtt.state());
      delay(2000); attempts++;
    }
  }
}