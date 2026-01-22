/// <reference types="vite/client" />

interface ImportMetaEnv {
  readonly VITE_API_BASE_URL?: string;
  readonly VITE_MQTT_WS_URL?: string;
  readonly VITE_PROXY_API?: string;
  readonly VITE_PROXY_WS?: string;
  readonly VITE_PROXY_MQTT?: string;
}

interface ImportMeta {
  readonly env: ImportMetaEnv;
}
