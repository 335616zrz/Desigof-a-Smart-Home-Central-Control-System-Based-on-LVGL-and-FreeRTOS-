import axios from 'axios';

const storedBase = typeof window !== 'undefined' ? localStorage.getItem('iot_api_base') : null;
const apiBase = storedBase || import.meta.env.VITE_API_BASE_URL || '/api';
const actuatorBase = apiBase.endsWith('/api') ? apiBase.replace(/\/api\/?$/, '') || '/' : apiBase;

const client = axios.create({
  baseURL: actuatorBase,
  timeout: 10000
});

export async function fetchActuatorLogfile(): Promise<string> {
  const { data } = await client.get('/actuator/logfile', { responseType: 'text' });
  return typeof data === 'string' ? data : String(data);
}
