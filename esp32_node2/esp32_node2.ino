#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP32Servo.h> // Thư viện điều khiển Servo cho ESP32

// --- CẤU HÌNH THÔNG TIN MẠNG & BROKER ---
const char* ssid = "Test";                // Thay tên Wi-Fi
const char* password = "12345678";         // Thay mật khẩu Wi-Fi
const char* mqtt_server = "192.168.137.1";  // IP máy tính chạy Mosquitto Broker

// --- CẤU HÌNH CẢM BIẾN & THIẾT BỊ ---
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define SERVO_PIN 16 // Chân GPIO kết nối tín hiệu Servo (ví dụ chân GPIO 13)
Servo escapeDoorServo;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

// --- HÀM NHẬN TIN NHẮN TỪ MQTT BROKER (LỆNH ĐIỀU KHIỂN) ---
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nhan duoc tin nhan từ topic [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Nếu có lệnh điều khiển cửa thoát hiểm
  if (String(topic) == "home/node2/control_escape_door") {
    if (message == "OPEN") {
      escapeDoorServo.write(90); // Quay góc 90 độ để mở cửa
      Serial.println("-> Da MO cua thoat hiem!");
    } else if (message == "CLOSE") {
      escapeDoorServo.write(0);  // Quay về góc 0 độ để đóng cửa
      Serial.println("-> Da DONG cua thoat hiem!");
    }
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Dang ket noi Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("-> Wi-Fi da ket noi!");
  Serial.print("IP cua ESP32: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Dang ket noi MQTT Broker...");
    String clientId = "ESP32_Node2_Client_";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println(" Thanh cong!");
      // Sau khi kết nối thành công, tiến hành đăng ký (subscribe) các topic cần lắng nghe
      client.subscribe("home/node2/control_escape_door");
    } else {
      Serial.print(" That bai, rc=");
      Serial.print(client.state());
      Serial.println(". Thu lai sau 5 giay...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  // Cài đặt chân cho Servo
  ESP32PWM::allocateTimer(0);
  escapeDoorServo.setPeriodHertz(50); // Tần số chuẩn cho Servo SG90
  escapeDoorServo.attach(SERVO_PIN, 500, 2400); // Khởi tạo chân Servo
  escapeDoorServo.write(0); // Mặc định ban đầu đóng cửa (0 độ)

  setup_wifi();
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); // Gắn hàm callback để nhận lệnh
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Đọc và gửi dữ liệu DHT11 mỗi 3 giây
  unsigned long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      String payload = "{\"temp\":" + String(t, 1) + ",\"humid\":" + String(h, 1) + "}";
      client.publish("home/sensors/dht11", payload.c_str());
      
      Serial.print("Da gui tin nhan MQTT DHT11: ");
      Serial.println(payload);
    } else {
      Serial.println("Loi doc cam bien DHT11!");
    }
  }
}