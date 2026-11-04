#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Wire.h>
#include <ESP32Servo.h>

// ===================== CẤU HÌNH WIFI & MQTT =====================
const char* ssid = "Test";
const char* password = "12345678";
const char* mqtt_server = "192.168.137.1";

WiFiClient espClient;
PubSubClient client(espClient);

// ===================== KHAI BÁO CHÂN =====================
#define RFID_SS    5
#define RFID_RST   16
#define RFID_SCK   18
#define RFID_MOSI  17
#define RFID_MISO  19

#define MPU_SDA    21
#define MPU_SCL    22
#define MPU_ADDR   0x68

#define SERVO_MAIN_PIN 4
#define SERVO_BED_PIN  15

#define LED_LIVING_PIN  23
#define LED_BEDROOM_PIN 2

// Mức sáng của đèn
#define LED_ACTIVE_LEVEL HIGH
const int LED_ON_LEVEL  = LED_ACTIVE_LEVEL;
const int LED_OFF_LEVEL = (LED_ACTIVE_LEVEL == HIGH) ? LOW : HIGH;

// ===================== CẤU HÌNH LOGIC =====================
const String PASSWORD_PIN = "1234";          // Mật khẩu keypad (ĐỔI LẠI)
const int    MAX_PIN_LEN  = 8;
const int    MAX_WRONG    = 3;
const unsigned long LOCKOUT_TIME = 30000;    // Khóa keypad 30 giây khi sai quá số lần

// Thẻ RFID hợp lệ (4 byte). Quẹt thẻ, xem UID trên Serial Monitor rồi điền vào đây
const byte AUTH_UIDS[][4] = {
  {0xDE, 0xAD, 0xBE, 0xEF},
  {0x12, 0x34, 0x56, 0x78},
  {0xE3, 0x12, 0xFF, 0x37},
  {0x87, 0xA9, 0x87, 0x25},
};
const int AUTH_COUNT = sizeof(AUTH_UIDS) / sizeof(AUTH_UIDS[0]);

const int SERVO_CLOSE_ANGLE = 0;
const int SERVO_OPEN_ANGLE  = 90;
const unsigned long DOOR_OPEN_TIME = 5000;   // Cửa chính tự đóng sau 5 giây (RFID/keypad)

// MPU6050 - phát hiện động đất
const float QUAKE_G_THRESHOLD = 0.15;          // Độ lệch so với 1g coi là "rung mạnh"
const int   QUAKE_HITS        = 5;             // Số lần rung mạnh trong 1 giây để báo động đất
const unsigned long QUAKE_HOLD_MS    = 10000;  // Hết rung 10 giây thì báo hết động đất
const unsigned long SERVO_IGNORE_MS  = 1500;   // Bỏ qua cảm biến sau khi servo chuyển động
const unsigned long MPU_INTERVAL     = 50;

const unsigned long MQTT_PUBLISH_INTERVAL = 3000;
const unsigned long MQTT_RETRY_INTERVAL   = 5000;
const unsigned long CARD_DEBOUNCE         = 1500;

// ===================== ĐỐI TƯỢNG =====================
MFRC522 rfid(RFID_SS, RFID_RST);
Servo mainServo;
Servo bedServo;

const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {27, 14, 12, 13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ===================== TRẠNG THÁI =====================
bool ledLivingOn   = false;
bool ledBedroomOn  = false;
bool mainOpen      = false;
bool bedOpen       = false;
bool quakeActive   = false;
bool mpuOk         = false;
bool publishNow    = false;
bool mainAutoClose = false;

String inputPin = "";
int    wrongCount = 0;
unsigned long lockUntil = 0;

unsigned long mainOpenedAt    = 0;
unsigned long lastServoMove   = 0;
unsigned long lastMpuTime     = 0;
unsigned long lastCardTime    = 0;
unsigned long lastMqttPubTime = 0;
unsigned long lastMqttRetry   = 0;

int   shakeHits = 0;
unsigned long hitWindowStart = 0;
unsigned long lastShakeTime  = 0;
float peakShake = 0;

// ===================== EVENT =====================
void publishEvent(const char* ev) {
  Serial.print("[EVENT] ");
  Serial.println(ev);
  if (client.connected()) {
    String p = "{\"event\":\"" + String(ev) + "\"}";
    client.publish("home/events/node1", p.c_str());
  }
}

// ===================== ĐIỀU KHIỂN THIẾT BỊ =====================
void setLedLiving(bool on) {
  ledLivingOn = on;
  digitalWrite(LED_LIVING_PIN, on ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

void setLedBedroom(bool on) {
  ledBedroomOn = on;
  digitalWrite(LED_BEDROOM_PIN, on ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

void openMain(bool autoClose) {
  if (mainOpen && !mainAutoClose) autoClose = false;   // Cửa đang giữ mở thì giữ nguyên
  mainServo.write(SERVO_OPEN_ANGLE);
  mainOpen = true;
  mainAutoClose = autoClose;
  mainOpenedAt = millis();
  lastServoMove = millis();
  publishNow = true;
}

void closeMain() {
  mainServo.write(SERVO_CLOSE_ANGLE);
  mainOpen = false;
  mainAutoClose = false;
  lastServoMove = millis();
  publishNow = true;
}

void openBed() {
  bedServo.write(SERVO_OPEN_ANGLE);
  bedOpen = true;
  lastServoMove = millis();
  publishNow = true;
}

void closeBed() {
  bedServo.write(SERVO_CLOSE_ANGLE);
  bedOpen = false;
  lastServoMove = millis();
  publishNow = true;
}

// ===================== ĐỘNG ĐẤT =====================
void triggerQuake() {
  quakeActive = true;
  lastShakeTime = millis();
  setLedLiving(true);
  setLedBedroom(true);
  openMain(false);     // Mở cửa chính và giữ nguyên để thoát hiểm
  openBed();
  publishEvent("earthquake");
  publishNow = true;
  Serial.println("[DONG DAT] Phat hien rung manh, mo cua thoat hiem!");
}

void clearQuake() {
  quakeActive = false;
  shakeHits = 0;
  publishEvent("quake_clear");
  publishNow = true;
  Serial.println("[OK] Het rung. Cua van mo, hay dong bang app.");
}

bool readMPU(float &ax, float &ay, float &az) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6) != 6) return false;
  int16_t x = (Wire.read() << 8) | Wire.read();
  int16_t y = (Wire.read() << 8) | Wire.read();
  int16_t z = (Wire.read() << 8) | Wire.read();
  ax = x / 16384.0;
  ay = y / 16384.0;
  az = z / 16384.0;
  return true;
}

void initMPU() {
  Wire.begin(MPU_SDA, MPU_SCL);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);   // Đánh thức MPU6050
  Wire.write(0x00);
  mpuOk = (Wire.endTransmission() == 0);
  if (!mpuOk) {
    Serial.println("Khong tim thay MPU6050 (kiem tra day SDA/SCL, dia chi 0x68)");
  }
}

// ===================== RFID =====================
bool isAuthorized() {
  if (rfid.uid.size != 4) return false;
  for (int i = 0; i < AUTH_COUNT; i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (rfid.uid.uidByte[j] != AUTH_UIDS[i][j]) { match = false; break; }
    }
    if (match) return true;
  }
  return false;
}
unsigned long lastRfidCheck = 0;

// Kiểm tra RC522 còn sống không, nếu treo thì khởi động lại
void rfidWatchdog() {
  unsigned long now = millis();
  if (now - lastRfidCheck < 3000) return;
  lastRfidCheck = now;

  byte v = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  if (v == 0x00 || v == 0xFF) {
    Serial.println("[RFID] RC522 mat ket noi hoac bi treo, khoi dong lai...");
    rfid.PCD_Init();
    rfid.PCD_SetAntennaGain(rfid.RxGain_max);
  }
}

void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  unsigned long now = millis();
  if (now - lastCardTime < CARD_DEBOUNCE) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  lastCardTime = now;

  Serial.print("UID the: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print("0x");
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  if (isAuthorized()) {
    Serial.println("[RFID] The hop le - mo cua chinh.");
    publishEvent("card_ok");
    openMain(true);
  } else {
    Serial.println("[RFID] The khong hop le!");
    publishEvent("card_denied");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ===================== KEYPAD =====================
void handleKey(char k) {
  unsigned long now = millis();

  if (now < lockUntil) {
    Serial.println("[KEYPAD] Dang bi khoa, vui long cho.");
    return;
  }

  if (k == '*') {
    inputPin = "";
    Serial.println("[KEYPAD] Da xoa.");
  }
  else if (k == '#') {
    if (inputPin == PASSWORD_PIN) {
      Serial.println("[KEYPAD] Mat khau dung - mo cua chinh.");
      publishEvent("pin_ok");
      wrongCount = 0;
      openMain(true);
    } else {
      wrongCount++;
      Serial.print("[KEYPAD] Sai mat khau lan ");
      Serial.println(wrongCount);
      publishEvent("pin_wrong");
      if (wrongCount >= MAX_WRONG) {
        lockUntil = now + LOCKOUT_TIME;
        wrongCount = 0;
        publishEvent("locked_out");
        Serial.println("[KEYPAD] Sai qua nhieu, khoa 30 giay.");
      }
    }
    inputPin = "";
  }
  else {
    if (inputPin.length() < MAX_PIN_LEN) inputPin += k;
    Serial.print("[KEYPAD] Dang nhap: ");
    for (unsigned int i = 0; i < inputPin.length(); i++) Serial.print("*");
    Serial.println();
  }
}

// ===================== WIFI =====================
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

  if (t == "home/node1/control_led_living") {
    if (message == "ON")       setLedLiving(true);
    else if (message == "OFF") setLedLiving(false);
  }
  else if (t == "home/node1/control_led_bedroom") {
    if (message == "ON")       setLedBedroom(true);
    else if (message == "OFF") setLedBedroom(false);
  }
  else if (t == "home/node1/control_main_door") {
    if (message == "OPEN")       openMain(false);   // Từ app: giữ mở cho đến khi bấm đóng
    else if (message == "CLOSE") closeMain();
  }
  else if (t == "home/node1/control_bedroom_door") {
    if (message == "OPEN")       openBed();
    else if (message == "CLOSE") closeBed();
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
  String clientId = "ESP32Node1-Home-";
  clientId += String(random(0xffff), HEX);

  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
    client.subscribe("home/node1/control_led_living");
    client.subscribe("home/node1/control_led_bedroom");
    client.subscribe("home/node1/control_main_door");
    client.subscribe("home/node1/control_bedroom_door");
    publishNow = true;
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" retry in 5 seconds");
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
    keypad.setDebounceTime(50);
  pinMode(LED_LIVING_PIN, OUTPUT);
  pinMode(LED_BEDROOM_PIN, OUTPUT);
  setLedLiving(false);
  setLedBedroom(false);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  mainServo.setPeriodHertz(50);
  bedServo.setPeriodHertz(50);
  mainServo.attach(SERVO_MAIN_PIN);
  bedServo.attach(SERVO_BED_PIN);
  mainServo.write(SERVO_CLOSE_ANGLE);
  bedServo.write(SERVO_CLOSE_ANGLE);
  lastServoMove = millis();

  SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI, RFID_SS);
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);   // Tăng độ nhạy ăng-ten
  Serial.print("RC522 version: 0x");
  Serial.println(rfid.PCD_ReadRegister(MFRC522::VersionReg), HEX);
  initMPU();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("=== NODE 1 - HOME MQTT READY ===");
}

// ===================== LOOP =====================
void loop() {
  if (!client.connected()) {
    reconnect();
  } else {
    client.loop();
  }

  unsigned long now = millis();

  // Keypad
    static unsigned long lastKeyTime = 0;
  char k = keypad.getKey();
  if (k && now - lastKeyTime > 250) {   // bỏ qua ký tự lặp trong 250ms
    lastKeyTime = now;
    Serial.print("Phim bam: ");
    Serial.println(k);
    handleKey(k);
  }
  rfidWatchdog();
  // RFID
  checkRFID();
  now = millis();   // Làm mới thời gian sau khi xử lý keypad/RFID
  // Cửa chính tự đóng sau khi mở bằng thẻ/mật khẩu
  if (mainOpen && mainAutoClose && now - mainOpenedAt >= DOOR_OPEN_TIME) {
    closeMain();
    Serial.println("[CUA] Tu dong dong cua chinh.");
  }

  // MPU6050 - phát hiện động đất
  if (mpuOk && now - lastMpuTime >= MPU_INTERVAL) {
    lastMpuTime = now;

    float ax, ay, az;
    if (readMPU(ax, ay, az)) {
      float dev = fabs(sqrt(ax * ax + ay * ay + az * az) - 1.0);
      bool ignore = (now - lastServoMove < SERVO_IGNORE_MS);

      if (!ignore) {
        if (dev > peakShake) peakShake = dev;

        if (now - hitWindowStart > 1000) {
          hitWindowStart = now;
          shakeHits = 0;
        }
        if (dev > QUAKE_G_THRESHOLD) {
          shakeHits++;
          lastShakeTime = now;
        }
        if (!quakeActive && shakeHits >= QUAKE_HITS) {
          triggerQuake();
        }
      }

      if (quakeActive && now - lastShakeTime > QUAKE_HOLD_MS) {
        clearQuake();
      }
    }
  }

  // Publish trạng thái (mỗi 3 giây hoặc ngay khi có thay đổi)
  if (client.connected() && (publishNow || now - lastMqttPubTime >= MQTT_PUBLISH_INTERVAL)) {
    lastMqttPubTime = now;
    publishNow = false;

    String payload = "{";
    payload += "\"led_living\":" + String(ledLivingOn ? 1 : 0) + ",";
    payload += "\"led_bedroom\":" + String(ledBedroomOn ? 1 : 0) + ",";
    payload += "\"main_door\":" + String(mainOpen ? 1 : 0) + ",";
    payload += "\"bedroom_door\":" + String(bedOpen ? 1 : 0) + ",";
    payload += "\"quake\":" + String(quakeActive ? 1 : 0) + ",";
    payload += "\"shake\":" + String(peakShake, 2);
    payload += "}";
    peakShake = 0;

    client.publish("home/sensors/node1_data", payload.c_str());
    Serial.println("-> Published Node1 Data to MQTT");
  }
}