#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===================== CẤU HÌNH WIFI & MQTT =====================
const char* ssid = "Test";
const char* password = "12345678";
const char* mqtt_server = "192.168.137.1";

WiFiClient espClient;
PubSubClient client(espClient);

// ===================== KHAI BÁO CHÂN & BIẾN =====================
#define DHTPIN 4           // DHT11 nhà bếp (dùng cho báo cháy)
#define DHT2PIN 14         // DHT11 phòng khách
#define DHTTYPE DHT11

#define MQ2_PIN 34
#define FLAME_DO_PIN 32

#define BUZZER_PIN 18
#define RELAY_PIN 19
#define LED_BEEP_PIN 23
#define FAN_PIN 25

#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// Mức kích của relay bơm: relay của bạn kích mức LOW
#define RELAY_ACTIVE_LEVEL LOW
const int RELAY_ON  = RELAY_ACTIVE_LEVEL;
const int RELAY_OFF = (RELAY_ACTIVE_LEVEL == HIGH) ? LOW : HIGH;

// Mức sáng của đèn: đèn của bạn sáng khi chân ở mức HIGH
#define LED_ACTIVE_LEVEL HIGH
const int LED_ON_LEVEL  = LED_ACTIVE_LEVEL;
const int LED_OFF_LEVEL = (LED_ACTIVE_LEVEL == HIGH) ? LOW : HIGH;

// Mức kích của quạt: HIGH nếu dùng transistor/MOSFET, đổi theo module nếu dùng relay
#define FAN_ACTIVE_LEVEL LOW
const int FAN_ON_LEVEL  = FAN_ACTIVE_LEVEL;
const int FAN_OFF_LEVEL = (FAN_ACTIVE_LEVEL == HIGH) ? LOW : HIGH;

const int MQ2_ON_THRESHOLD  = 2200;
const int MQ2_OFF_THRESHOLD = 2000;
const unsigned long MQ2_WARMUP_TIME = 30000;

const bool  USE_TEMP_ALARM     = true;
const float TEMP_ON_THRESHOLD  = 32.0;
const float TEMP_OFF_THRESHOLD = 30.0;

// Ngưỡng nhiệt độ phòng khách để bật/tắt quạt
const float FAN_ON_THRESHOLD  = 34.0;
const float FAN_OFF_THRESHOLD = 32.0;

const unsigned long HOLD_TIME             = 10000;
const unsigned long SENSOR_INTERVAL       = 200;
const unsigned long DHT_INTERVAL          = 2000;
const unsigned long OLED_INTERVAL         = 1000;
const unsigned long MQTT_PUBLISH_INTERVAL = 3000;
const unsigned long MQTT_RETRY_INTERVAL   = 5000;

const int FLAME_CONFIRM_COUNT = 3;

DHT dht(DHTPIN, DHTTYPE);
DHT dht2(DHT2PIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool isAlarmTriggered = false;
bool buzzerMuted  = false;
bool buzzerManual = false;

// Trạng thái thật của thiết bị (gửi lên app)
bool ledOn    = true;
bool pumpOn   = false;
bool buzzerOn = false;
bool fanOn    = false;

bool publishNow = false;

unsigned long lastDangerTime  = 0;
unsigned long lastSensorTime  = 0;
unsigned long lastDhtTime     = 0;
unsigned long lastOledTime    = 0;
unsigned long lastMqttPubTime = 0;
unsigned long lastMqttRetry   = 0;

int   flameLowCount = 0;
float temperature   = 0;
float humidity      = 0;
bool  tempValid     = false;
int   mq2Value      = 0;
bool  flameDetected = false;

// Phòng khách
float roomTemp  = 0;
float roomHum   = 0;
bool  roomValid = false;
bool  roomHot   = false;   // Đang ở trạng thái "nóng" (để bật quạt đúng lúc vượt ngưỡng)
bool  oledOk    = false;

// ===================== HÀM ĐIỀU KHIỂN THIẾT BỊ =====================
void setLed(bool on) {
  ledOn = on;
  digitalWrite(LED_BEEP_PIN, on ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

void setPump(bool on) {
  pumpOn = on;
  digitalWrite(RELAY_PIN, on ? RELAY_ON : RELAY_OFF);
}

void setFan(bool on) {
  fanOn = on;
  digitalWrite(FAN_PIN, on ? FAN_ON_LEVEL : FAN_OFF_LEVEL);
}

void setEmergency(bool on) {
  if (on) {
    buzzerMuted = false;
    setPump(true);
    setLed(false);
  } else {
    buzzerMuted = false;
    buzzerManual = false;
    setPump(false);
  }
  publishNow = true;
}

// ===================== OLED =====================
void updateOled() {
  if (!oledOk) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (isAlarmTriggered) {
    display.setTextSize(2);
    display.setCursor(0, 8);
    display.println("CANH BAO!");
    display.setCursor(0, 32);
    display.println("CHAY / KHOI");
  } else {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("PHONG KHACH");

    display.setTextSize(3);
    display.setCursor(0, 16);
    if (roomValid) display.print(String(roomTemp, 0));
    else           display.print("--");
    display.print("C");

    display.setTextSize(1);
    display.setCursor(0, 46);
    display.print("Do am: ");
    if (roomValid) display.print(String(roomHum, 0));
    else           display.print("--");
    display.print("%");

    display.setCursor(0, 56);
    display.print("Quat: ");
    display.print(fanOn ? "BAT" : "TAT");
  }

  display.display();
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
  else if (t == "home/node2/control_fan") {
    if (message == "ON")       setFan(true);
    else if (message == "OFF") setFan(false);
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

  publishNow = true;
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
    client.subscribe("home/node2/control_fan");
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
  pinMode(FAN_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);

  setPump(false);
  setLed(true);
  setFan(false);

  dht.begin();
  dht2.begin();

  Wire.begin(OLED_SDA, OLED_SCL);
  oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!oledOk) {
    Serial.println("Khong tim thay OLED (kiem tra day SDA/SCL, dia chi 0x3C)");
  } else {
    display.clearDisplay();
    display.display();
  }

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

      // DHT11 nhà bếp
      float t = dht.readTemperature();
      float h = dht.readHumidity();
      if (!isnan(t) && !isnan(h)) {
        temperature = t;
        humidity = h;
        tempValid = true;
      } else {
        tempValid = false;
      }

      // DHT11 phòng khách + logic quạt
      float rt = dht2.readTemperature();
      float rh = dht2.readHumidity();
      if (!isnan(rt) && !isnan(rh)) {
        roomTemp = rt;
        roomHum = rh;
        roomValid = true;

        if (!roomHot && roomTemp > FAN_ON_THRESHOLD) {
          roomHot = true;
          setFan(true);
          publishNow = true;
          Serial.println("[QUAT] Phong khach nong, bat quat.");
        } else if (roomHot && roomTemp < FAN_OFF_THRESHOLD) {
          roomHot = false;
          setFan(false);
          publishNow = true;
          Serial.println("[QUAT] Phong khach mat, tat quat.");
        }
      } else {
        roomValid = false;
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

  // Cập nhật OLED mỗi 1 giây
  if (now - lastOledTime >= OLED_INTERVAL) {
    lastOledTime = now;
    updateOled();
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
    payload += "\"buzzer\":" + String(buzzerOn ? 1 : 0) + ",";
    payload += "\"room_temperature\":" + String(roomTemp) + ",";
    payload += "\"room_humidity\":" + String(roomHum) + ",";
    payload += "\"fan\":" + String(fanOn ? 1 : 0);
    payload += "}";

    client.publish("home/sensors/kitchen_data", payload.c_str());
    Serial.println("-> Published Kitchen Data to MQTT");
  }
}
