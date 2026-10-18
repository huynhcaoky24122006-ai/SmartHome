#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ===================== CẤU HÌNH WIFI & MQTT =====================
const char* ssid = "Test";
const char* password = "12345678";
const char* mqtt_server = "192.168.137.1";

WiFiClient espClient;
PubSubClient client(espClient);

// ===================== KHAI BÁO CHÂN & BIẾN =====================
#define DHTPIN 4
#define DHTTYPE DHT11

#define MQ2_PIN 34
#define FLAME_DO_PIN 32

#define BUZZER_PIN 18
#define RELAY_PIN 19
#define LED_BEEP_PIN 23

// Mức kích của relay: relay của bạn kích mức LOW
#define RELAY_ACTIVE_LEVEL LOW
const int RELAY_ON  = RELAY_ACTIVE_LEVEL;
const int RELAY_OFF = (RELAY_ACTIVE_LEVEL == HIGH) ? LOW : HIGH;

// Mức sáng của đèn: đèn của bạn sáng khi chân ở mức HIGH
#define LED_ACTIVE_LEVEL HIGH
const int LED_ON_LEVEL  = LED_ACTIVE_LEVEL;
const int LED_OFF_LEVEL = (LED_ACTIVE_LEVEL == HIGH) ? LOW : HIGH;

const int MQ2_ON_THRESHOLD  = 1900;
const int MQ2_OFF_THRESHOLD = 1800;
const unsigned long MQ2_WARMUP_TIME = 30000;

const bool  USE_TEMP_ALARM     = true;
const float TEMP_ON_THRESHOLD  = 32.0;
const float TEMP_OFF_THRESHOLD = 30.0;

const unsigned long HOLD_TIME             = 10000;
const unsigned long SENSOR_INTERVAL       = 200;
const unsigned long DHT_INTERVAL          = 2000;
const unsigned long MQTT_PUBLISH_INTERVAL = 3000;
const unsigned long MQTT_RETRY_INTERVAL   = 5000;

const int FLAME_CONFIRM_COUNT = 3;

DHT dht(DHTPIN, DHTTYPE);

bool isAlarmTriggered = false;
bool buzzerMuted  = false;   // App tắt tiếng còi trong lúc báo động
bool buzzerManual = false;   // App bật còi thủ công

// Trạng thái thật của thiết bị (gửi lên app)
bool ledOn    = true;
bool pumpOn   = false;
bool buzzerOn = false;

bool publishNow = false;     // Gửi trạng thái ngay khi có thay đổi

unsigned long lastDangerTime  = 0;
unsigned long lastSensorTime  = 0;
unsigned long lastDhtTime     = 0;
unsigned long lastMqttPubTime = 0;
unsigned long lastMqttRetry   = 0;

int   flameLowCount = 0;
float temperature   = 0;
float humidity      = 0;
bool  tempValid     = false;
int   mq2Value      = 0;
bool  flameDetected = false;

// ===================== HÀM ĐIỀU KHIỂN THIẾT BỊ =====================
void setLed(bool on) {
  ledOn = on;
  digitalWrite(LED_BEEP_PIN, on ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

void setPump(bool on) {
  pumpOn = on;
  digitalWrite(RELAY_PIN, on ? RELAY_ON : RELAY_OFF);
}

void setEmergency(bool on) {
  if (on) {
    buzzerMuted = false;
    setPump(true);      // Bật máy bơm
    setLed(false);      // Tắt đèn
  } else {
    buzzerMuted = false;
    buzzerManual = false;
    setPump(false);     // Tắt máy bơm
  }
  publishNow = true;
}

// ===================== WIFI (chỉ dùng lúc khởi động) =====================
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ===================== MQTT CALLBACK =====================
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("MQTT Received [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  String t = String(topic);

  if (t == "home/node2/control_led") {
    if (message == "ON")       setLed(true);
    else if (message == "OFF") setLed(false);
  }
  else if (t == "home/node2/control_pump") {
    if (message == "ON")       setPump(true);
    else if (message == "OFF") setPump(false);
  }
  else if (t == "home/node2/control_buzzer") {
    if (message == "ON") {
      buzzerManual = true;
      buzzerMuted = false;
    } else if (message == "OFF") {
      buzzerManual = false;
      buzzerMuted = true;
    }
  }

  publishNow = true;   // Báo trạng thái mới cho app ngay
}

// ===================== RECONNECT KHÔNG CHẶN LOOP =====================
void reconnect() {
  unsigned long now = millis();
  if (now - lastMqttRetry < MQTT_RETRY_INTERVAL) return;
  lastMqttRetry = now;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
    return;
  }

  Serial.print("Attempting MQTT connection...");
  String clientId = "ESP32Node2-Kitchen-";
  clientId += String(random(0xffff), HEX);

  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
    client.subscribe("home/node2/control_led");
    client.subscribe("home/node2/control_pump");
    client.subscribe("home/node2/control_buzzer");
    publishNow = true;
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" retry in 5 seconds");
  }
}

int readMQ2() {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(MQ2_PIN);
    delay(2);
  }
  return sum / 10;
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  pinMode(MQ2_PIN, INPUT);
  pinMode(FLAME_DO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_BEEP_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);

  setPump(false);
  setLed(true);
  dht.begin();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("=== NODE 2 - KITCHEN MQTT READY ===");
}

// ===================== LOOP =====================
void loop() {
  if (!client.connected()) {
    reconnect();
  } else {
    client.loop();
  }

  unsigned long now = millis();

  // Còi hú liên tục khi có báo động
  buzzerOn = (isAlarmTriggered && !buzzerMuted) || buzzerManual;
  digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);

  // Đọc cảm biến định kỳ
  if (now - lastSensorTime >= SENSOR_INTERVAL) {
    lastSensorTime = now;

    mq2Value = readMQ2();
    bool mq2Ready = (now >= MQ2_WARMUP_TIME);

    if (digitalRead(FLAME_DO_PIN) == LOW) {
      if (flameLowCount < 255) flameLowCount++;
    } else {
      flameLowCount = 0;
    }
    flameDetected = (flameLowCount >= FLAME_CONFIRM_COUNT);

    if (now - lastDhtTime >= DHT_INTERVAL) {
      lastDhtTime = now;
      float t = dht.readTemperature();
      float h = dht.readHumidity();
      if (!isnan(t) && !isnan(h)) {
        temperature = t;
        humidity = h;
        tempValid = true;
      } else {
        tempValid = false;
      }
    }

    bool mq2Danger = mq2Ready && (mq2Value > MQ2_ON_THRESHOLD);
    bool mq2Safe   = !mq2Ready || (mq2Value < MQ2_OFF_THRESHOLD);

    bool fireAndHighTempDanger = flameDetected && USE_TEMP_ALARM && tempValid && (temperature > TEMP_ON_THRESHOLD);
    bool fireAndHighTempSafe   = !flameDetected || !USE_TEMP_ALARM || !tempValid || (temperature < TEMP_OFF_THRESHOLD);

    bool danger = mq2Danger || fireAndHighTempDanger;
    bool safe   = mq2Safe && fireAndHighTempSafe;

    if (danger) {
      lastDangerTime = now;
      if (!isAlarmTriggered) {
        isAlarmTriggered = true;
        setEmergency(true);
        Serial.println("[CANH BAO] KICH HOAT UNG CUU KHAN CAP (Phat hien khoi hoac chay lon)!");
      }
    }
    else if (safe && isAlarmTriggered) {
      if (now - lastDangerTime >= HOLD_TIME) {
        isAlarmTriggered = false;
        setEmergency(false);
        Serial.println("[OK] Moi truong an toan, tro lai binh thuong.");
      }
    }
  }

  // Publish dữ liệu + trạng thái thiết bị (mỗi 3 giây hoặc ngay khi có thay đổi)
  if (client.connected() && (publishNow || now - lastMqttPubTime >= MQTT_PUBLISH_INTERVAL)) {
    lastMqttPubTime = now;
    publishNow = false;

    String payload = "{";
    payload += "\"temperature\":" + String(temperature) + ",";
    payload += "\"humidity\":" + String(humidity) + ",";
    payload += "\"smoke\":" + String(mq2Value) + ",";
    payload += "\"flame\":" + String(flameDetected ? 1 : 0) + ",";
    payload += "\"alarm\":" + String(isAlarmTriggered ? 1 : 0) + ",";
    payload += "\"led\":" + String(ledOn ? 1 : 0) + ",";
    payload += "\"pump\":" + String(pumpOn ? 1 : 0) + ",";
    payload += "\"buzzer\":" + String(buzzerOn ? 1 : 0);
    payload += "}";

    client.publish("home/sensors/kitchen_data", payload.c_str());
    Serial.println("-> Published Kitchen Data to MQTT");
  }
}