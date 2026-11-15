# 🏠 SmartHome

Hệ thống nhà thông minh gồm các **node ESP32** thu thập dữ liệu / điều khiển thiết bị, một **backend** trung tâm xử lý và lưu trữ, và một **ứng dụng** để người dùng giám sát, điều khiển từ xa.

> ⚠️ Các mục đánh dấu `TODO` cần bạn bổ sung thông tin thực tế của project.

---

## 📌 Tính năng

- Giám sát dữ liệu từ các node ESP32 theo thời gian thực `TODO: nhiệt độ, độ ẩm, ...`
- Điều khiển thiết bị từ xa qua ứng dụng `TODO: đèn, quạt, cửa, ...`
- Backend trung gian nhận / lưu trữ dữ liệu và cung cấp API cho ứng dụng
- Mở rộng dễ dàng bằng cách thêm node ESP32 mới

## 🧩 Kiến trúc hệ thống

```
┌──────────────┐        ┌──────────────┐
│ ESP32 Node 1 │        │ ESP32 Node 2 │
└──────┬───────┘        └──────┬───────┘
       │   TODO: MQTT / HTTP / WebSocket
       └───────────┬───────────┘
                   ▼
          ┌─────────────────┐
          │ smarthome-backend│
          └────────┬────────┘
                   │   TODO: REST API / WebSocket
                   ▼
          ┌─────────────────┐
          │  SmartHomeApp   │
          └─────────────────┘
```

## 📁 Cấu trúc thư mục

```
SmartHome/
├── SmartHomeApp/        # Ứng dụng người dùng (TODO: Flutter / React Native / Android ...)
├── esp32_node1/         # Firmware cho ESP32 node 1 (TODO: vai trò của node)
├── esp32_node2/         # Firmware cho ESP32 node 2 (TODO: vai trò của node)
├── smarthome-backend/   # Server backend (TODO: Node.js / Python ...)
└── .gitignore
```

| Thư mục | Mô tả |
|---|---|
| `SmartHomeApp` | Ứng dụng giám sát và điều khiển nhà thông minh |
| `esp32_node1` | Firmware node 1 `TODO: cảm biến / thiết bị kết nối` |
| `esp32_node2` | Firmware node 2 `TODO: cảm biến / thiết bị kết nối` |
| `smarthome-backend` | Backend nhận dữ liệu từ node, lưu trữ và cung cấp API |

## 🛠️ Công nghệ sử dụng

| Thành phần | Công nghệ |
|---|---|
| Vi điều khiển | ESP32 `TODO: Arduino framework / ESP-IDF / PlatformIO` |
| Backend | `TODO` |
| Ứng dụng | `TODO` |
| Giao thức | `TODO: MQTT / HTTP / WebSocket` |
| Cơ sở dữ liệu | `TODO` |

## 🔌 Phần cứng

| Linh kiện | Số lượng | Ghi chú |
|---|---|---|
| ESP32 DevKit | 2 | Mỗi node một board |
| `TODO: cảm biến` | | |
| `TODO: relay / module` | | |

Sơ đồ nối dây: `TODO: thêm ảnh/sơ đồ vào thư mục docs/`

## 🚀 Cài đặt & chạy

### 1. Clone repo

```bash
git clone https://github.com/huynhcaoky24122006-ai/SmartHome.git
cd SmartHome
```

### 2. Backend

```bash
cd smarthome-backend
# TODO: lệnh cài đặt, ví dụ: npm install
# TODO: lệnh chạy, ví dụ: npm start
```

## 👤 Tác giả

- GitHub: [@huynhcaoky24122006-ai](https://github.com/huynhcaoky24122006-ai)

## 📄 Giấy phép

