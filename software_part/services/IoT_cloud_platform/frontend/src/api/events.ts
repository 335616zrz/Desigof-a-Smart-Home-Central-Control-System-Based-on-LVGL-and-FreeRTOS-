export function openEventStream(base?: string) {
  const apiBase = base || localStorage.getItem('iot_api_base') || import.meta.env.VITE_API_BASE_URL || '/api';
  const root = apiBase.replace(/\/api\/?$/, '');
  const url = `${root.replace(/\/$/, '')}/api/events/stream`;
  return new EventSource(url);
}
