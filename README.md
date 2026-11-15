# 🏠 SmartHome

Hệ thống nhà thông minh gồm các **node ESP32** điều khiển thiết bị và thu thập dữ liệu qua **MQTT**, một **backend** trung tâm xử lý và lưu trữ, và một **ứng dụng** để người dùng giám sát, điều khiển từ xa.

> ⚠️ Các mục đánh dấu `TODO` cần bạn bổ sung thông tin thực tế của project (đặc biệt là Node 2, Backend và App).

---

## 📌 Tính năng

**Node 1 – Cửa & an ninh**
- 🔑 Mở cửa chính bằng **thẻ RFID** (danh sách thẻ hợp lệ lưu trong code) hoặc **mật khẩu keypad**
- 🔒 Khóa keypad 30 giây khi nhập sai mật khẩu 3 lần
- 🚪 Cửa chính tự đóng sau 5 giây khi mở bằng thẻ/mật khẩu; mở từ app thì giữ mở đến khi bấm đóng
- 🌍 **Phát hiện động đất** bằng cảm biến MPU6050: tự bật đèn, mở cửa chính và cửa phòng ngủ để thoát hiểm, gửi cảnh báo qua MQTT
- 💡 Điều khiển đèn phòng khách, đèn phòng ngủ và cửa phòng ngủ từ xa qua MQTT
- 📡 Báo trạng thái định kỳ (3 giây) hoặc ngay khi có thay đổi; tự kết nối lại Wi-Fi/MQTT không chặn vòng lặp chính; watchdog tự khởi động lại module RC522 khi bị treo

**Node 2**
- `TODO`

**Backend / App**
- `TODO`

## 🧩 Kiến trúc hệ thống

```
┌──────────────┐        ┌──────────────┐
│ ESP32 Node 1 │        │ ESP32 Node 2 │
└──────┬───────┘        └──────┬───────┘
       │         MQTT (1883)   │
       └───────────┬───────────┘
                   ▼
          ┌─────────────────┐
          │   MQTT Broker   │
          └────────┬────────┘
                   ▼
          ┌──────────────────┐
          │ smarthome-backend │   TODO: cách backend nhận dữ liệu / lưu trữ
          └────────┬─────────┘
                   ▼
          ┌─────────────────┐
          │  SmartHomeApp   │   TODO: REST API / WebSocket
          └─────────────────┘
```

## 📁 Cấu trúc thư mục

```
SmartHome/
├── SmartHomeApp/        # Ứng dụng người dùng (TODO: Flutter / React Native / Android ...)
├── esp32_node1/         # Firmware node 1: cửa, RFID, keypad, cảm biến động đất
├── esp32_node2/         # Firmware node 2 (TODO)
├── smarthome-backend/   # Server backend (TODO: Node.js / Python ...)
└── .gitignore
```

| Thư mục | Mô tả |
|---|---|
| `SmartHomeApp` | Ứng dụng giám sát và điều khiển nhà thông minh |
| `esp32_node1` | Firmware node 1: RFID, keypad, MPU6050, 2 servo cửa, 2 đèn |
| `esp32_node2` | Firmware node 2 `TODO: vai trò của node` |
| `smarthome-backend` | Backend nhận dữ liệu từ node, lưu trữ và cung cấp API |

## 🛠️ Công nghệ sử dụng

| Thành phần | Công nghệ |
|---|---|
| Vi điều khiển | ESP32 (Arduino framework) |
| Giao thức | MQTT (broker cổng 1883) |
| Thư viện ESP32 | `WiFi`, `PubSubClient`, `MFRC522`, `Keypad`, `ESP32Servo`, `SPI`, `Wire` |
| Backend | `TODO` |
| Ứng dụng | `TODO` |
| Cơ sở dữ liệu | `TODO` |

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

`TODO`

Sơ đồ nối dây: `TODO: thêm ảnh/sơ đồ vào thư mục docs/`

## 🚀 Cài đặt & chạy

### 1. Clone repo

```bash
git clone https://github.com/huynhcaoky24122006-ai/SmartHome.git
cd SmartHome
```

### 2. MQTT Broker

Cần một MQTT broker (ví dụ Mosquitto) chạy trên máy tính/server cùng mạng với các node, cổng `1883`.

### 3. Firmware ESP32

1. Cài **Arduino IDE** và thêm board **ESP32**.
2. Cài các thư viện qua Library Manager: `PubSubClient`, `MFRC522`, `Keypad`, `ESP32Servo`.
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

`TODO: hướng dẫn nạp esp32_node2`

### 4. Backend

```bash
cd smarthome-backend
# TODO: lệnh cài đặt, ví dụ: npm install
# TODO: lệnh chạy, ví dụ: npm start
```

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

`TODO`

## 🌍 Cơ chế phát hiện động đất (Node 1)

- MPU6050 được đọc mỗi 50 ms, tính độ lệch của tổng gia tốc so với 1g.
- Nếu độ lệch vượt **0.15 g** từ **5 lần trở lên trong 1 giây** → báo động đất.
- Khi báo động đất: bật cả hai đèn, mở cửa chính và cửa phòng ngủ, gửi sự kiện `earthquake`.
- Sau **10 giây** không còn rung → gửi `quake_clear`. Cửa vẫn mở, người dùng đóng lại bằng app.
- Cảm biến bị bỏ qua 1.5 giây sau mỗi lần servo chuyển động để tránh báo nhầm do rung cơ khí.

## 🔐 Lưu ý bảo mật

- Mật khẩu keypad, UID thẻ và thông tin Wi-Fi đang đặt trực tiếp trong code. **Hãy đổi trước khi sử dụng thật** và không commit thông tin thật lên repo công khai.
- Kết nối MQTT hiện chưa dùng tài khoản/mật khẩu hay TLS. Nếu đưa ra ngoài mạng LAN, nên bật xác thực cho broker.

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
