const mqtt = require('mqtt');
const { initializeApp, cert } = require('firebase-admin/app');
const { getDatabase } = require('firebase-admin/database');

const serviceAccount = require("./firebase-key.json");

initializeApp({
  credential: cert(serviceAccount),
  databaseURL: "https://smarthome-44dbf-default-rtdb.firebaseio.com"
});

const db = getDatabase();
const mqttClient = mqtt.connect('mqtt://192.168.137.1:1883');

mqttClient.on('connect', () => {
  console.log('Node.js Server đã kết nối thành công tới MQTT Broker!');
  
  // Subscribe topic dữ liệu cảm biến nhà bếp
  mqttClient.subscribe('home/sensors/kitchen_data', (err) => {
    if (!err) console.log('Đã subscribe topic: home/sensors/kitchen_data');
  });
});

// LẮNG NGHE LỆNH ĐIỀU KHIỂN TỪ FIREBASE ĐỂ BẮN XUỐNG MQTT CHO ESP32
db.ref('controls/kitchen').on('value', (snapshot) => {
  const controls = snapshot.val();
  if (controls) {
    if (controls.led) {
      mqttClient.publish('home/node2/control_led', controls.led);
      console.log(`[MQTT Send] home/node2/control_led -> ${controls.led}`);
    }
    if (controls.pump) {
      mqttClient.publish('home/node2/control_pump', controls.pump);
      console.log(`[MQTT Send] home/node2/control_pump -> ${controls.pump}`);
    }
    if (controls.escapeDoor) {
      mqttClient.publish('home/node2/control_escape_door', controls.escapeDoor);
      console.log(`[MQTT Send] home/node2/control_escape_door -> ${controls.escapeDoor}`);
    }
    if (controls.buzzer) {
      mqttClient.publish('home/node2/control_buzzer', controls.buzzer);
      console.log(`[MQTT Send] home/node2/control_buzzer -> ${controls.buzzer}`);
    }
  }
});

mqttClient.on('message', async (topic, message) => {
  try {
    const data = JSON.parse(message.toString());
    console.log(`Bắt được gói tin từ [${topic}]:`, data);

    if (topic === 'home/sensors/kitchen_data') {
      await db.ref('sensors/kitchen').set({
        temperature: data.temperature,
        humidity: data.humidity,
        smoke: data.smoke,
        flame: data.flame,
        alarm: data.alarm,
        updatedAt: Date.now()
      });
      console.log("Đã đẩy dữ liệu Nhà Bếp lên Firebase thành công!");
    }
  } catch (error) {
    console.error("Lỗi xử lý dữ liệu MQTT:", error);
  }
});