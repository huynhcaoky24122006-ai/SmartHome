const mqtt = require('mqtt');
const { initializeApp, cert } = require('firebase-admin/app');
const { getDatabase } = require('firebase-admin/database');

// 1. Cấu hình kết nối Firebase Realtime Database
const serviceAccount = require("./firebase-key.json");

initializeApp({
  credential: cert(serviceAccount),
  databaseURL: "https://smarthome-44dbf-default-rtdb.firebaseio.com"
});

const db = getDatabase();

// 2. Kết nối tới Mosquitto MQTT Broker (Dùng IP Hotspot 192.168.137.1)
const mqttClient = mqtt.connect('mqtt://192.168.137.1:1883');

mqttClient.on('connect', () => {
  console.log('Node.js Server đã kết nối thành công tới MQTT Broker!');
  
  // Đăng ký nhận dữ liệu từ Topic của DHT11
  mqttClient.subscribe('home/sensors/dht11', (err) => {
    if (!err) {
      console.log('Đã subscribe topic: home/sensors/dht11');
    }
  });
});

// 3. Lắng nghe dữ liệu từ MQTT gửi về và lưu lên Firebase
mqttClient.on('message', async (topic, message) => {
  if (topic === 'home/sensors/dht11') {
    try {
      const data = JSON.parse(message.toString());
      console.log("Bắt được gói tin từ ESP32:", data);

      // Lưu/Cập nhật dữ liệu vào Node 'sensors/dht11' trên Firebase
      await db.ref('sensors/dht11').set({
        temperature: data.temp,
        humidity: data.humid,
        updatedAt: Date.now()
      });

      console.log("Đã cập nhật thành công lên Firebase Realtime Database!");
    } catch (error) {
      console.error("Lỗi ghi dữ liệu hoặc giải mã JSON:", error);
    }
  }
});