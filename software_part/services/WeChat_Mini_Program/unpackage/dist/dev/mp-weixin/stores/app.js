"use strict";
const stores_pinia = require("./pinia.js");
const useAppStore = stores_pinia.defineStore("app", {
  state: () => ({
    // Network state ("wifi", "4g", "none", ...).
    networkType: "unknown",
    // MQTT connection status ("connected", "disconnected", "unknown").
    mqttStatus: "unknown",
    // Current season for template selection (optional, backend-driven later).
    season: ""
  }),
  actions: {
    setNetworkType(type) {
      this.networkType = type || "unknown";
    },
    setMqttStatus(status) {
      this.mqttStatus = status || "unknown";
    },
    setSeason(season) {
      this.season = season || "";
    }
  }
});
exports.useAppStore = useAppStore;
//# sourceMappingURL=../../.sourcemap/mp-weixin/stores/app.js.map
