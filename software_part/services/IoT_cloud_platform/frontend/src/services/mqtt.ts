import mqtt, { MqttClient } from 'mqtt';
import { useTelemetryStream } from '@/stores/telemetryStream';
import { useEventStore } from '@/stores/event';

let client: MqttClient | null = null;

export function initMqtt(options: { host?: string }) {
  if (client) return client;
  const telemetry = useTelemetryStream();
  const events = useEventStore();

  // 构建完整的 WebSocket URL
  let mqttUrl = options.host?.trim() || '';

  // 如果是相对路径，转换为绝对 WebSocket URL
  if (mqttUrl.startsWith('/')) {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    mqttUrl = `${protocol}//${window.location.host}${mqttUrl}`;
  } else if (!mqttUrl) {
    // 如果没有提供 URL，使用环境变量或默认值
    const envUrl = import.meta.env.VITE_MQTT_WS_URL;
    if (envUrl && envUrl.startsWith('/')) {
      const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
      mqttUrl = `${protocol}//${window.location.host}${envUrl}`;
    } else {
      mqttUrl = envUrl ?? 'ws://localhost:8083/mqtt';
    }
  }

  console.log('MQTT connecting to:', mqttUrl);

  client = mqtt.connect(mqttUrl, {
    reconnectPeriod: 2000,
    clientId: `web-${Math.random().toString(16).slice(2)}`
  });

  client.on('connect', () => {
    telemetry.setConnected(true);
    client?.subscribe('devices/+/state');
    events.push({
      id: `sys-${Date.now()}`,
      type: 'system',
      message: 'MQTT 连接成功',
      timestamp: Date.now()
    });
  });

  client.on('message', (topic, message) => {
    const deviceId = topic.split('/')[1];
    try {
      const payload = JSON.parse(message.toString());
      // Extract sensor data from payload.data if it exists, otherwise use top-level
      const sensorData = payload.data || payload;
      telemetry.push(deviceId, sensorData);
      events.push({
        id: `${deviceId}-${Date.now()}`,
        type: 'telemetry',
        deviceId,
        message: `设备 ${deviceId} 上报数据`,
        payload: sensorData,
        timestamp: Date.now()
      });
    } catch (err) {
      console.warn('MQTT parse error', err);
    }
  });

  client.on('close', () => {
    telemetry.setConnected(false);
    events.push({
      id: `sys-close-${Date.now()}`,
      type: 'system',
      message: 'MQTT 连接关闭',
      timestamp: Date.now()
    });
  });
  client.on('error', () => {
    telemetry.setConnected(false);
    events.push({
      id: `sys-error-${Date.now()}`,
      type: 'system',
      message: 'MQTT 连接异常',
      timestamp: Date.now()
    });
  });
  return client;
}

export function disconnectMqtt() {
  client?.end(true);
  client = null;
}
