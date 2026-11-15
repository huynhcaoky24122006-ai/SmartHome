# 🏠 SmartHome

Hệ thống nhà thông minh gồm các **node ESP32** điều khiển thiết bị và thu thập dữ liệu qua **MQTT**, một **backend** trung tâm xử lý và lưu trữ, và một **ứng dụng** để người dùng giám sát, điều khiển từ xa.



---

## 📌 Tính năng

**Node 1 – Cửa & an ninh**
- 🔑 Mở cửa chính bằng **thẻ RFID** (danh sách thẻ hợp lệ lưu trong code) hoặc **mật khẩu keypad**
- 🔒 Khóa keypad 30 giây khi nhập sai mật khẩu 3 lần
- 🚪 Cửa chính tự đóng sau 5 giây khi mở bằng thẻ/mật khẩu; mở từ app thì giữ mở đến khi bấm đóng
- 🌍 **Phát hiện động đất** bằng cảm biến MPU6050: tự bật đèn, mở cửa chính và cửa phòng ngủ để thoát hiểm, gửi cảnh báo qua MQTT
- 💡 Điều khiển đèn phòng khách, đèn phòng ngủ và cửa phòng ngủ từ xa qua MQTT
- 📡 Báo trạng thái định kỳ (3 giây) hoặc ngay khi có thay đổi; tự kết nối lại Wi-Fi/MQTT không chặn vòng lặp chính; watchdog tự khởi động lại module RC522 khi bị treo

**Node 2 – Nhà bếp & phòng khách**
- 🔥 **Cảnh báo cháy / khói**: dùng cảm biến khí MQ-2, cảm biến lửa và nhiệt độ nhà bếp
- 🚨 Khi báo động: tự bật **máy bơm** (relay), bật **còi** hú liên tục, tắt LED và hiển thị cảnh báo trên OLED
- 🌡️ Đo nhiệt độ, độ ẩm **nhà bếp** và **phòng khách** (2 cảm biến DHT11)
- 🌀 Tự bật/tắt **quạt** theo nhiệt độ phòng khách (có ngưỡng trễ để tránh bật tắt liên tục)
- 📟 Màn hình OLED hiển thị nhiệt độ, độ ẩm phòng khách và trạng thái quạt
- 📡 Điều khiển LED, máy bơm, quạt, còi từ xa qua MQTT; báo trạng thái mỗi 3 giây hoặc ngay khi có thay đổi

**Backend**
- Cầu nối **MQTT ↔ Firebase Realtime Database** viết bằng Node.js
- Nhận dữ liệu cảm biến và trạng thái từ các node qua MQTT, đẩy lên Firebase
- Nhận lệnh điều khiển mới từ Firebase (do app ghi), gửi xuống node tương ứng qua MQTT
- Ghi lại lịch sử sự kiện của Node 1 (quẹt thẻ, nhập mật khẩu, động đất...) lên Firebase

**App**
- `TODO`

## 🧩 Kiến trúc hệ thống

```
┌──────────────┐        ┌──────────────┐
│ ESP32 Node 1 │        │ ESP32 Node 2 │
└──────┬───────┘        └──────┬───────┘
       │         MQTT (1883)   │
       └───────────┬───────────┘
                   ↕
          ┌─────────────────┐
          │   MQTT Broker   │
          └────────┬────────┘
                   ↕
          ┌───────────────────┐
          │ smarthome-backend │   Node.js: cầu nối MQTT ↔ Firebase
          └─────────┬─────────┘
                    ↕
          ┌───────────────────┐
          │ Firebase Realtime │
          │     Database      │
          └─────────┬─────────┘
                    ↕
          ┌───────────────────┐
          │   SmartHomeApp    │
          └───────────────────┘
```

**Luồng dữ liệu**
- **Giám sát:** Node → MQTT → Backend → Firebase → App
- **Điều khiển:** App ghi lệnh vào Firebase → Backend nhận → MQTT → Node thực thi

## 📁 Cấu trúc thư mục

```
SmartHome/
├── SmartHomeApp/        # Ứng dụng người dùng (TODO: Flutter / React Native / Android ...)
├── esp32_node1/         # Firmware node 1: cửa, RFID, keypad, cảm biến động đất
├── esp32_node2/         # Firmware node 2: báo cháy/khói, đo nhiệt độ, quạt, máy bơm
├── smarthome-backend/   # Backend Node.js: cầu nối MQTT ↔ Firebase
└── .gitignore
```

| Thư mục | Mô tả |
|---|---|
| `SmartHomeApp` | Ứng dụng giám sát và điều khiển nhà thông minh |
| `esp32_node1` | Firmware node 1: RFID, keypad, MPU6050, 2 servo cửa, 2 đèn |
| `esp32_node2` | Firmware node 2: MQ-2, cảm biến lửa, 2 DHT11, OLED, relay bơm/quạt, còi |
| `smarthome-backend` | Backend Node.js (`server.js`): chuyển dữ liệu giữa MQTT và Firebase Realtime Database |

## 🛠️ Công nghệ sử dụng

| Thành phần | Công nghệ |
|---|---|
| Vi điều khiển | ESP32 (Arduino framework) |
| Giao thức | MQTT (broker cổng 1883) |
| Thư viện ESP32 | `WiFi`, `PubSubClient`, `MFRC522`, `Keypad`, `ESP32Servo`, `DHT`, `Adafruit_GFX`, `Adafruit_SSD1306`, `SPI`, `Wire` |
| Backend | Node.js (CommonJS), thư viện `mqtt`, `firebase-admin` |
| Ứng dụng | `TODO` |
| Cơ sở dữ liệu | Firebase Realtime Database |

## 🔌 Phần cứng

### Node 1

| Linh kiện | Số lượng | Ghi chú |
|---|---|---|
| ESP32 DevKit | 1 | |
| Module RFID RC522 | 1 | Giao tiếp SPI |
| Keypad 4x4 | 1 | |
| MPU6050 | 1 | Giao tiếp I2C, địa chỉ `0x68` |
| Servo | 2 | Cửa chính và cửa phòng ngủ |
| LED | 2 | Phòng khách và phòng ngủ |

**Sơ đồ chân Node 1**

| Thiết bị | Chân ESP32 |
|---|---|
| RC522 – SS / RST | GPIO 5 / GPIO 16 |
| RC522 – SCK / MOSI / MISO | GPIO 18 / GPIO 17 / GPIO 19 |
| MPU6050 – SDA / SCL | GPIO 21 / GPIO 22 |
| Servo cửa chính | GPIO 4 |
| Servo cửa phòng ngủ | GPIO 15 |
| LED phòng khách | GPIO 23 |
| LED phòng ngủ | GPIO 2 |
| Keypad – hàng (R1–R4) | GPIO 32, 33, 25, 26 |
| Keypad – cột (C1–C4) | GPIO 27, 14, 12, 13 |

### Node 2

| Linh kiện | Số lượng | Ghi chú |
|---|---|---|
| ESP32 DevKit | 1 | |
| Cảm biến khí MQ-2 | 1 | Đọc analog |
| Cảm biến lửa | 1 | Dùng chân DO (mức LOW khi phát hiện lửa) |
| DHT11 | 2 | Nhà bếp và phòng khách |
| OLED SSD1306 128x64 | 1 | Giao tiếp I2C, địa chỉ `0x3C` |
| Relay | 2 | Máy bơm và quạt |
| Còi (buzzer) | 1 | |
| LED | 1 | |

**Sơ đồ chân Node 2**

| Thiết bị | Chân ESP32 |
|---|---|
| DHT11 nhà bếp | GPIO 32 |
| DHT11 phòng khách | GPIO 33 |
| MQ-2 (analog) | GPIO 34 |
| Cảm biến lửa (DO) | GPIO 35 |
| Còi | GPIO 13 |
| Relay máy bơm (IN2) | GPIO 26 |
| Relay quạt (IN1) | GPIO 25 |
| LED | GPIO 12 |
| OLED – SDA / SCL | GPIO 27 / GPIO 14 |

Mức kích của relay bơm, relay quạt và LED được cấu hình bằng các hằng số `RELAY_ACTIVE_LEVEL`, `FAN_ACTIVE_LEVEL`, `LED_ACTIVE_LEVEL` trong code. Hãy đổi cho khớp với module của bạn.

Sơ đồ nối dây: `TODO: thêm ảnh/sơ đồ vào thư mục docs/`

## 🚀 Cài đặt & chạy

### 1. Clone repo

```bash
git clone https://github.com/huynhcaoky24122006-ai/SmartHome.git
cd SmartHome
```

### 2. MQTT Broker

Cần một MQTT broker (ví dụ Mosquitto) chạy trên máy tính/server cùng mạng với các node và backend, cổng `1883`. Địa chỉ broker phải khớp với `mqtt_server` trong firmware ESP32 và địa chỉ trong `smarthome-backend/server.js`.

### 3. Firmware ESP32

1. Cài **Arduino IDE** và thêm board **ESP32**.
2. Cài các thư viện qua Library Manager: `PubSubClient`, `MFRC522`, `Keypad`, `ESP32Servo`, `DHT sensor library`, `Adafruit GFX Library`, `Adafruit SSD1306`.
3. Mở `esp32_node1/esp32_node1.ino` và sửa phần cấu hình:

   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   const char* mqtt_server = "YOUR_BROKER_IP";   // IP máy chạy MQTT broker

   const String PASSWORD_PIN = "YOUR_PIN";       // Mật khẩu keypad

   const byte AUTH_UIDS[][4] = { ... };          // UID các thẻ RFID hợp lệ
   ```

4. Chọn board **ESP32 Dev Module**, chọn đúng cổng COM và nạp code.
5. Để lấy UID thẻ mới: mở Serial Monitor (115200 baud), quẹt thẻ, copy UID in ra rồi thêm vào `AUTH_UIDS`.

Với Node 2: mở `esp32_node2/esp32_node2.ino`, sửa `ssid`, `password`, `mqtt_server` tương tự Node 1 rồi nạp code. Có thể chỉnh các ngưỡng báo động ngay trong code (xem phần *Cơ chế báo cháy* bên dưới).

### 4. Backend

Yêu cầu: **Node.js** và một project **Firebase** có bật **Realtime Database**.

1. Trong Firebase Console, vào *Project settings → Service accounts → Generate new private key*, tải file JSON về và đổi tên thành `firebase-key.json`.
2. Đặt `firebase-key.json` vào thư mục `smarthome-backend/` (**không commit file này lên GitHub**).
3. Mở `smarthome-backend/server.js` và sửa cho khớp với hệ thống của bạn:

   ```js
   databaseURL: "https://<your-project-id>-default-rtdb.firebaseio.com"
   const mqttClient = mqtt.connect('mqtt://<IP_MQTT_BROKER>:1883');
   ```

4. Cài thư viện và chạy:

   ```bash
   cd smarthome-backend
   npm install
   node server.js
   ```

Khi chạy thành công, console sẽ in `Node.js Server đã kết nối thành công tới MQTT Broker!` và `Sẵn sàng nhận lệnh điều khiển từ app.`

### 5. Ứng dụng

```bash
cd SmartHomeApp
# TODO: lệnh cài đặt & chạy
```

## 📡 MQTT Topics

### Node 1

**Điều khiển (node subscribe)**

| Topic | Payload | Chức năng |
|---|---|---|
| `home/node1/control_led_living` | `ON` / `OFF` | Bật/tắt đèn phòng khách |
| `home/node1/control_led_bedroom` | `ON` / `OFF` | Bật/tắt đèn phòng ngủ |
| `home/node1/control_main_door` | `OPEN` / `CLOSE` | Mở/đóng cửa chính (mở từ app thì giữ mở đến khi đóng) |
| `home/node1/control_bedroom_door` | `OPEN` / `CLOSE` | Mở/đóng cửa phòng ngủ |

**Dữ liệu (node publish)**

| Topic | Tần suất | Payload |
|---|---|---|
| `home/sensors/node1_data` | Mỗi 3 giây hoặc khi trạng thái thay đổi | JSON trạng thái |
| `home/events/node1` | Khi có sự kiện | `{"event": "<tên sự kiện>"}` |

Ví dụ payload `home/sensors/node1_data`:

```json
{
  "led_living": 0,
  "led_bedroom": 0,
  "main_door": 0,
  "bedroom_door": 0,
  "quake": 0,
  "shake": 0.02
}
```

| Trường | Ý nghĩa |
|---|---|
| `led_living`, `led_bedroom` | Trạng thái đèn (1 = bật) |
| `main_door`, `bedroom_door` | Trạng thái cửa (1 = mở) |
| `quake` | 1 = đang có động đất |
| `shake` | Độ rung lớn nhất (đơn vị g) kể từ lần gửi trước |

Các sự kiện trên `home/events/node1`:

| Sự kiện | Ý nghĩa |
|---|---|
| `card_ok` / `card_denied` | Quẹt thẻ hợp lệ / không hợp lệ |
| `pin_ok` / `pin_wrong` | Nhập đúng / sai mật khẩu |
| `locked_out` | Keypad bị khóa do sai quá số lần |
| `earthquake` | Phát hiện động đất |
| `quake_clear` | Hết rung (cửa vẫn mở, cần đóng bằng app) |

### Node 2

**Điều khiển (node subscribe)**

| Topic | Payload | Chức năng |
|---|---|---|
| `home/node2/control_led` | `ON` / `OFF` | Bật/tắt LED |
| `home/node2/control_pump` | `ON` / `OFF` | Bật/tắt máy bơm |
| `home/node2/control_fan` | `ON` / `OFF` | Bật/tắt quạt |
| `home/node2/control_buzzer` | `ON` / `OFF` | `ON`: bật còi thủ công; `OFF`: tắt (tắt tiếng) còi |

**Dữ liệu (node publish)**

| Topic | Tần suất | Payload |
|---|---|---|
| `home/sensors/kitchen_data` | Mỗi 3 giây hoặc khi trạng thái thay đổi | JSON dữ liệu cảm biến và trạng thái thiết bị |

Ví dụ payload `home/sensors/kitchen_data`:

```json
{
  "temperature": 28.00,
  "humidity": 65.00,
  "smoke": 820,
  "flame": 0,
  "alarm": 0,
  "led": 1,
  "pump": 0,
  "buzzer": 0,
  "room_temperature": 30.00,
  "room_humidity": 60.00,
  "fan": 0
}
```

| Trường | Ý nghĩa |
|---|---|
| `temperature`, `humidity` | Nhiệt độ (°C), độ ẩm (%) nhà bếp |
| `smoke` | Giá trị analog MQ-2 (trung bình 10 lần đọc) |
| `flame` | 1 = phát hiện lửa |
| `alarm` | 1 = đang báo động cháy/khói |
| `led`, `pump`, `buzzer`, `fan` | Trạng thái thiết bị (1 = bật) |
| `room_temperature`, `room_humidity` | Nhiệt độ (°C), độ ẩm (%) phòng khách |

## 🗄️ Firebase Realtime Database

Backend đọc/ghi các nhánh sau. App cần **đọc** `sensors` và `events`, **ghi** vào `controls`.

```
sensors/
├── kitchen/        temperature, humidity, smoke, flame, alarm, led, pump, buzzer,
│                   room_temperature, room_humidity, fan, updatedAt
└── node1/          led_living, led_bedroom, main_door, bedroom_door, quake, shake, updatedAt
events/
└── node1/<id>/     event, ts              (mỗi sự kiện là một bản ghi mới)
controls/
├── kitchen/        led | pump | fan | buzzer
└── node1/          ledLiving | ledBedroom | mainDoor | bedroomDoor
```

**Ánh xạ lệnh điều khiển (Firebase → MQTT)**

| Firebase | Giá trị | MQTT topic |
|---|---|---|
| `controls/kitchen/led` | `ON` / `OFF` | `home/node2/control_led` |
| `controls/kitchen/pump` | `ON` / `OFF` | `home/node2/control_pump` |
| `controls/kitchen/fan` | `ON` / `OFF` | `home/node2/control_fan` |
| `controls/kitchen/buzzer` | `ON` / `OFF` | `home/node2/control_buzzer` |
| `controls/node1/ledLiving` | `ON` / `OFF` | `home/node1/control_led_living` |
| `controls/node1/ledBedroom` | `ON` / `OFF` | `home/node1/control_led_bedroom` |
| `controls/node1/mainDoor` | `OPEN` / `CLOSE` | `home/node1/control_main_door` |
| `controls/node1/bedroomDoor` | `OPEN` / `CLOSE` | `home/node1/control_bedroom_door` |

Lưu ý:
- Giá trị có thể ghi trực tiếp (`"ON"`) hoặc dạng object có trường `cmd` (`{"cmd": "ON"}`).
- Khi khởi động, backend bỏ qua các lệnh đã có sẵn trong Firebase và chỉ gửi những lệnh **mới thay đổi** sau đó.

## 🔥 Cơ chế báo cháy và điều khiển quạt (Node 2)

**Báo động** được kích hoạt khi một trong hai điều kiện sau xảy ra:

- Giá trị MQ-2 vượt **1600** (chỉ xét sau 30 giây khởi động để cảm biến làm nóng), hoặc
- Cảm biến lửa phát hiện lửa (3 lần đọc liên tiếp) **và** nhiệt độ nhà bếp trên **34 °C**.

Khi báo động: bật máy bơm, tắt LED, còi hú liên tục, OLED hiển thị "CANH BAO!".

**Tắt báo động** khi MQ-2 xuống dưới **1400** và nhiệt độ nhà bếp xuống dưới **32 °C** (hoặc không còn lửa), và môi trường an toàn liên tục trong **10 giây**. Lúc đó máy bơm tắt và còi dừng.

**Quạt** tự bật khi nhiệt độ phòng khách trên **40 °C** và tự tắt khi xuống dưới **39 °C**.

Các ngưỡng này nằm ở đầu file `esp32_node2.ino` (`MQ2_ON_THRESHOLD`, `TEMP_ON_THRESHOLD`, `FAN_ON_THRESHOLD`, `HOLD_TIME`...), có thể chỉnh theo thực tế.

## 🌍 Cơ chế phát hiện động đất (Node 1)

- MPU6050 được đọc mỗi 50 ms, tính độ lệch của tổng gia tốc so với 1g.
- Nếu độ lệch vượt **0.15 g** từ **5 lần trở lên trong 1 giây** → báo động đất.
- Khi báo động đất: bật cả hai đèn, mở cửa chính và cửa phòng ngủ, gửi sự kiện `earthquake`.
- Sau **10 giây** không còn rung → gửi `quake_clear`. Cửa vẫn mở, người dùng đóng lại bằng app.
- Cảm biến bị bỏ qua 1.5 giây sau mỗi lần servo chuyển động để tránh báo nhầm do rung cơ khí.

## 🔐 Lưu ý bảo mật

- Mật khẩu keypad, UID thẻ và thông tin Wi-Fi đang đặt trực tiếp trong code. **Hãy đổi trước khi sử dụng thật** và không commit thông tin thật lên repo công khai.
- Kết nối MQTT hiện chưa dùng tài khoản/mật khẩu hay TLS. Nếu đưa ra ngoài mạng LAN, nên bật xác thực cho broker.
- `smarthome-backend/firebase-key.json` là khóa quản trị Firebase: **tuyệt đối không commit**, hãy thêm vào `.gitignore` (cùng `node_modules/`). Nếu lỡ đẩy lên GitHub, hãy thu hồi và tạo khóa mới trong Firebase Console.
- Cấu hình **Firebase Security Rules** để chỉ người dùng đã đăng nhập mới đọc/ghi được `sensors`, `events` và `controls`, vì các lệnh này điều khiển cửa và thiết bị trong nhà.

## 🖼️ Hình ảnh demo

`TODO: thêm ảnh chụp ứng dụng, mô hình thực tế`

## 🗺️ Hướng phát triển

- [ ] `TODO`
- [ ] Bảo mật MQTT (xác thực, TLS)
- [ ] Thêm node / thiết bị mới

## 👤 Tác giả

- GitHub: [@huynhcaoky24122006-ai](https://github.com/huynhcaoky24122006-ai)

## 📄 Giấy phép

`TODO: chọn license, ví dụ MIT`
