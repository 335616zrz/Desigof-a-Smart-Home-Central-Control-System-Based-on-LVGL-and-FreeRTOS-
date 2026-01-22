import axios from 'axios';

const storedBase = typeof window !== 'undefined' ? localStorage.getItem('iot_api_base') : null;
const apiBase = storedBase || import.meta.env.VITE_API_BASE_URL || '/api';
// 如果 baseURL 以 /api 结尾，去掉该前缀以便访问 actuator
const actuatorBase = apiBase.endsWith('/api') ? apiBase.replace(/\/api\/?$/, '') || '/' : apiBase;

const monitor = axios.create({
  baseURL: actuatorBase,
  timeout: 12000
});

export async function fetchHealth() {
  const { data } = await monitor.get('/actuator/health');
  return data;
}

export async function fetchMetric(name: string) {
  const { data } = await monitor.get(`/actuator/metrics/${name}`);
  return data;
}

export async function fetchMetrics(names: string[]) {
  return Promise.all(
    names.map(async n => {
      try {
        const data = await fetchMetric(n);
        return { name: n, data };
      } catch (err) {
        return { name: n, error: err };
      }
    })
  );
}
