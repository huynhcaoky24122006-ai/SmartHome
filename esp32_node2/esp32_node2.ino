#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP32Servo.h>

// ===================== CẤU HÌNH WIFI & MQTT =====================
const char* ssid = "Test";       // Thay bằng tên wifi hotspot laptop
const char* password = "12345678";   // Thay bằng mật khẩu wifi
const char* mqtt_server = "192.168.137.1";     // IP Mosquitto Broker trên laptop

WiFiClient espClient;
PubSubClient client(espClient);

// ===================== KHAI BÁO CHÂN & BIẾN CŨ =====================
#define DHTPIN 4
#define DHTTYPE DHT11

#define MQ2_PIN 34         
#define FLAME_DO_PIN 32    

#define BUZZER_PIN 18
#define RELAY_PIN 19
#define LED_BEEP_PIN 23    
#define SERVO_PIN 13

#define RELAY_ACTIVE_LEVEL HIGH
const int RELAY_ON  = RELAY_ACTIVE_LEVEL;
const int RELAY_OFF = (RELAY_ACTIVE_LEVEL == HIGH) ? LOW : HIGH;

const int MQ2_ON_THRESHOLD  = 2200;                
const int MQ2_OFF_THRESHOLD = 2000;                
const unsigned long MQ2_WARMUP_TIME = 30000;      

const bool  USE_TEMP_ALARM     = true;
const float TEMP_ON_THRESHOLD  = 50.0;            
const float TEMP_OFF_THRESHOLD = 45.0;            

const unsigned long HOLD_TIME       = 10000;      
const unsigned long SENSOR_INTERVAL = 200;        
const unsigned long DHT_INTERVAL    = 2000;       
const unsigned long PRINT_INTERVAL  = 1000;       
const unsigned long MQTT_PUBLISH_INTERVAL = 3000; // Gửi dữ liệu lên MQTT mỗi 3 giây

const int FLAME_CONFIRM_COUNT = 3;

const int SERVO_CLOSE_ANGLE = 0;
const int SERVO_OPEN_ANGLE  = 90;

DHT dht(DHTPIN, DHTTYPE);
Servo escapeServo;

bool isAlarmTriggered = false;
unsigned long lastDangerTime = 0;  
unsigned long lastSensorTime = 0;
unsigned long lastDhtTime    = 0;
unsigned long lastPrintTime  = 0;
unsigned long lastMqttPubTime = 0;

int   flameLowCount = 0;
float temperature   = 0;
float humidity      = 0;
bool  tempValid     = false;
int   mq2Value      = 0;
bool  flameDetected = false;

// Hàm kết nối WiFi
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

// Hàm nhận lệnh điều khiển từ MQTT (Bật/tắt đèn, bơm, mở cửa thủ công từ App)
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("MQTT Received [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  // Điều khiển đèn bếp từ xa
  if (String(topic) == "home/node2/control_led") {
    if (message == "ON") {
      digitalWrite(LED_BEEP_PIN, LOW); 
    } else if (message == "OFF") {
      digitalWrite(LED_BEEP_PIN, HIGH);
    }
  }
  // Điều khiển máy bơm từ xa
  else if (String(topic) == "home/node2/control_pump") {
    if (message == "ON") digitalWrite(RELAY_PIN, RELAY_ON);
    else if (message == "OFF") digitalWrite(RELAY_PIN, RELAY_OFF);
  }
  // Điều khiển cửa thoát hiểm từ xa
  else if (String(topic) == "home/node2/control_escape_door") {
    if (message == "OPEN") escapeServo.write(SERVO_OPEN_ANGLE);
    else if (message == "CLOSE") escapeServo.write(SERVO_CLOSE_ANGLE);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Node2-Kitchen-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Đăng ký nhận các topic điều khiển
      client.subscribe("home/node2/control_led");
      client.subscribe("home/node2/control_pump");
      client.subscribe("home/node2/control_escape_door");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
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

void setEmergency(bool on) {
  digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
  digitalWrite(RELAY_PIN, on ? RELAY_ON : RELAY_OFF);
  digitalWrite(LED_BEEP_PIN, on ? HIGH : LOW); // Khi báo động thì tắt đèn (hoặc tùy chỉnh)
  escapeServo.write(on ? SERVO_OPEN_ANGLE : SERVO_CLOSE_ANGLE);
}

void setup() {
  Serial.begin(115200);

  pinMode(MQ2_PIN, INPUT);
  pinMode(FLAME_DO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_BEEP_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, RELAY_OFF);

  ESP32PWM::allocateTimer(0);
  escapeServo.setPeriodHertz(50);
  escapeServo.attach(SERVO_PIN);

  setEmergency(false); 
  dht.begin();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("=== NODE 2 - KITCHEN MQTT READY ===");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

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

    // Đọc DHT11
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

    // Đánh giá mức độ an toàn / nguy hiểm (Edge Logic)
    bool mq2Danger  = mq2Ready && (mq2Value > MQ2_ON_THRESHOLD);
    bool mq2Safe    = !mq2Ready || (mq2Value < MQ2_OFF_THRESHOLD);
    bool tempDanger = USE_TEMP_ALARM && tempValid && (temperature > TEMP_ON_THRESHOLD);
    bool tempSafe   = !USE_TEMP_ALARM || !tempValid || (temperature < TEMP_OFF_THRESHOLD);

    bool danger = flameDetected || mq2Danger || tempDanger;
    bool safe   = !flameDetected && mq2Safe && tempSafe;

    if (danger) {
      lastDangerTime = now;  
      if (!isAlarmTriggered) {
        isAlarmTriggered = true;
        setEmergency(true);
        Serial.println("[CANH BAO] KICH HOAT UNG CUU KHAN CAP!");
      }
    }
    else if (safe && isAlarmTriggered) {
      if (now - lastDangerTime >= HOLD_TIME) {
        isAlarmTriggered = false;
        setEmergency(false);
        Serial.println("[OK] Moi truong an toan, tro lai binh thuong.");
      }
    }

    // Publish dữ liệu cảm biến lên MQTT định kỳ mỗi 3 giây
    if (now - lastMqttPubTime >= MQTT_PUBLISH_INTERVAL) {
      lastMqttPubTime = now;

      String payload = "{";
      payload += "\"temperature\":" + String(temperature) + ",";
      payload += "\"humidity\":" + String(humidity) + ",";
      payload += "\"smoke\":" + String(mq2Value) + ",";
      payload += "\"flame\":" + String(flameDetected ? 1 : 0) + ",";
      payload += "\"alarm\":" + String(isAlarmTriggered ? 1 : 0);
      payload += "}";

      client.publish("home/sensors/kitchen_data", payload.c_str());
      Serial.println("-> Published Kitchen Data to MQTT");
    }
  }
}