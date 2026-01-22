import { defineStore } from './pinia.js'

export const useAppStore = defineStore('app', {
  state: () => ({
    // Network state ("wifi", "4g", "none", ...).
    networkType: 'unknown',
    // MQTT connection status ("connected", "disconnected", "unknown").
    mqttStatus: 'unknown',
    // Current season for template selection (optional, backend-driven later).
    season: '',
  }),
  actions: {
    setNetworkType(type) {
      this.networkType = type || 'unknown'
    },
    setMqttStatus(status) {
      this.mqttStatus = status || 'unknown'
    },
    setSeason(season) {
      this.season = season || ''
    },
  },
})

