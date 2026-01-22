"use strict";
const api_device = require("../api/device.js");
const stores_pinia = require("./pinia.js");
function unwrap(resp) {
  return (resp == null ? void 0 : resp.data) ?? resp ?? null;
}
const useDeviceStore = stores_pinia.defineStore("device", {
  state: () => ({
    items: [],
    total: 0,
    loading: false,
    current: null
  }),
  actions: {
    async fetchList(params) {
      this.loading = true;
      try {
        const resp = await api_device.listDevices(params);
        const data = unwrap(resp);
        this.items = (data == null ? void 0 : data.items) || [];
        this.total = (data == null ? void 0 : data.total) || 0;
        return data;
      } finally {
        this.loading = false;
      }
    },
    async fetchDetail(deviceId) {
      const resp = await api_device.getDevice(deviceId);
      const data = unwrap(resp);
      this.current = data;
      return data;
    },
    async create(payload) {
      const resp = await api_device.createDevice(payload);
      const data = unwrap(resp);
      return data;
    },
    async update(deviceId, payload) {
      const resp = await api_device.updateDevice(deviceId, payload);
      const data = unwrap(resp);
      return data;
    },
    async remove(deviceId) {
      await api_device.deleteDevice(deviceId);
      this.items = (this.items || []).filter((d) => (d == null ? void 0 : d.deviceId) !== deviceId);
      this.total = Math.max(0, (this.total || 0) - 1);
    },
    async provision(deviceId, payload) {
      const resp = await api_device.provisionDevice(deviceId, payload);
      return resp;
    }
  }
});
exports.useDeviceStore = useDeviceStore;
//# sourceMappingURL=../../.sourcemap/mp-weixin/stores/device.js.map
