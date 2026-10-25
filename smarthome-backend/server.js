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
  mqttClient.subscribe('home/sensors/kitchen_data', (err) => {
    if (!err) console.log('Đã subscribe topic: home/sensors/kitchen_data');
  });
});

mqttClient.on('error', (err) => {
  console.error('Lỗi MQTT:', err.message);
});

// Ánh xạ key trên Firebase -> topic MQTT
const COMMAND_TOPICS = {
  led: 'home/node2/control_led',
  pump: 'home/node2/control_pump',
  fan: 'home/node2/control_fan',
  buzzer: 'home/node2/control_buzzer',
};

// Bỏ qua dữ liệu cũ đã lưu sẵn lúc server khởi động, chỉ gửi lệnh MỚI
let ready = false;

function handleControl(snapshot) {
  if (!ready) return;

  const topic = COMMAND_TOPICS[snapshot.key];
  if (!topic) return;

  const val = snapshot.val();
  const cmd = (val && typeof val === 'object') ? val.cmd : val;
  if (!cmd) return;

  mqttClient.publish(topic, String(cmd));
  console.log(`[MQTT Send] ${topic} -> ${cmd}`);
}

const controlsRef = db.ref('controls/kitchen');
controlsRef.on('child_added', handleControl);
controlsRef.on('child_changed', handleControl);
controlsRef.once('value').then(() => {
  ready = true;
  console.log('Sẵn sàng nhận lệnh điều khiển từ app.');
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
        led: data.led ?? 0,
        pump: data.pump ?? 0,
        buzzer: data.buzzer ?? 0,
        room_temperature: data.room_temperature ?? 0,
        room_humidity: data.room_humidity ?? 0,
        fan: data.fan ?? 0,
        updatedAt: Date.now()
      });
      console.log("Đã đẩy dữ liệu Nhà Bếp lên Firebase thành công!");
    }
  } catch (error) {
    console.error("Lỗi xử lý dữ liệu MQTT:", error);
  }
});
