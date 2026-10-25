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
  const topics = [
    'home/sensors/kitchen_data',
    'home/sensors/node1_data',
    'home/events/node1',
  ];
  mqttClient.subscribe(topics, (err) => {
    if (!err) console.log('Đã subscribe:', topics.join(', '));
  });
});

mqttClient.on('error', (err) => {
  console.error('Lỗi MQTT:', err.message);
});

// Ánh xạ key trên Firebase -> topic MQTT, chia theo từng node
const COMMAND_TOPICS = {
  kitchen: {
    led: 'home/node2/control_led',
    pump: 'home/node2/control_pump',
    fan: 'home/node2/control_fan',
    buzzer: 'home/node2/control_buzzer',
  },
  node1: {
    ledLiving: 'home/node1/control_led_living',
    ledBedroom: 'home/node1/control_led_bedroom',
    mainDoor: 'home/node1/control_main_door',
    bedroomDoor: 'home/node1/control_bedroom_door',
  },
};

// Bỏ qua dữ liệu cũ đã lưu sẵn lúc server khởi động, chỉ gửi lệnh MỚI
let ready = false;

function makeHandler(group) {
  return (snapshot) => {
    if (!ready) return;

    const topic = COMMAND_TOPICS[group][snapshot.key];
    if (!topic) return;

    const val = snapshot.val();
    const cmd = (val && typeof val === 'object') ? val.cmd : val;
    if (!cmd) return;

    mqttClient.publish(topic, String(cmd));
    console.log(`[MQTT Send] ${topic} -> ${cmd}`);
  };
}

const watchers = Object.keys(COMMAND_TOPICS).map((group) => {
  const r = db.ref(`controls/${group}`);
  const handler = makeHandler(group);
  r.on('child_added', handler);
  r.on('child_changed', handler);
  return r.once('value');
});

Promise.all(watchers).then(() => {
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
    else if (topic === 'home/sensors/node1_data') {
      await db.ref('sensors/node1').set({
        led_living: data.led_living ?? 0,
        led_bedroom: data.led_bedroom ?? 0,
        main_door: data.main_door ?? 0,
        bedroom_door: data.bedroom_door ?? 0,
        quake: data.quake ?? 0,
        shake: data.shake ?? 0,
        updatedAt: Date.now()
      });
      console.log("Đã đẩy dữ liệu Node 1 lên Firebase thành công!");
    }
    else if (topic === 'home/events/node1') {
      await db.ref('events/node1').push({
        event: data.event,
        ts: Date.now()
      });
      console.log(`Đã ghi sự kiện Node 1: ${data.event}`);
    }
  } catch (error) {
    console.error("Lỗi xử lý dữ liệu MQTT:", error);
  }
});