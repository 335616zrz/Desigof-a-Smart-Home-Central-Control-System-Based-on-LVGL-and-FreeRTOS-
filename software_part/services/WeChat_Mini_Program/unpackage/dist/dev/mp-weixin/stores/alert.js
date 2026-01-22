"use strict";
const api_alert = require("../api/alert.js");
const stores_pinia = require("./pinia.js");
const useAlertStore = stores_pinia.defineStore("alert", {
  state: () => ({
    items: [],
    total: 0,
    page: 0,
    pageSize: 20,
    loading: false,
    current: null
  }),
  actions: {
    async fetchList(params) {
      this.loading = true;
      try {
        const resp = await api_alert.listAlerts(params);
        this.items = (resp == null ? void 0 : resp.items) || [];
        this.total = (resp == null ? void 0 : resp.total) || 0;
        this.page = (resp == null ? void 0 : resp.page) || 0;
        this.pageSize = (resp == null ? void 0 : resp.pageSize) || 20;
        return resp;
      } finally {
        this.loading = false;
      }
    },
    async fetchDetail(id) {
      const resp = await api_alert.getAlert(id);
      this.current = resp || null;
      return this.current;
    },
    async acknowledge(id) {
      await api_alert.acknowledgeAlert(id);
      const idx = (this.items || []).findIndex((a) => (a == null ? void 0 : a.id) === id);
      if (idx >= 0)
        this.items[idx].status = "ack";
      if (this.current && this.current.id === id)
        this.current.status = "ack";
    },
    async close(id) {
      await api_alert.closeAlert(id);
      const idx = (this.items || []).findIndex((a) => (a == null ? void 0 : a.id) === id);
      if (idx >= 0)
        this.items[idx].status = "closed";
      if (this.current && this.current.id === id)
        this.current.status = "closed";
    }
  }
});
exports.useAlertStore = useAlertStore;
//# sourceMappingURL=../../.sourcemap/mp-weixin/stores/alert.js.map
