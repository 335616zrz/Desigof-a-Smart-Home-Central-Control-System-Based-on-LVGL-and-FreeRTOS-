"use strict";
const api_alertRule = require("../api/alertRule.js");
const stores_pinia = require("./pinia.js");
const useAlertRuleStore = stores_pinia.defineStore("alertRule", {
  state: () => ({
    rules: [],
    userRules: [],
    templates: [],
    season: null,
    loading: false,
    current: null
  }),
  actions: {
    async fetchRules(enabled) {
      this.loading = true;
      try {
        const resp = await api_alertRule.listAlertRules(enabled);
        this.rules = resp || [];
        return this.rules;
      } finally {
        this.loading = false;
      }
    },
    async fetchUserRules(deviceId) {
      const resp = await api_alertRule.listUserRules(deviceId);
      this.userRules = resp || [];
      return this.userRules;
    },
    async fetchCurrentSeason() {
      const resp = await api_alertRule.getCurrentSeason();
      this.season = resp || null;
      return this.season;
    },
    async fetchTemplates(season) {
      const resp = await api_alertRule.listSystemTemplates(season);
      this.templates = resp || [];
      return this.templates;
    },
    async fetchDetail(id) {
      const resp = await api_alertRule.getAlertRule(id);
      this.current = resp || null;
      return this.current;
    },
    async create(payload) {
      const resp = await api_alertRule.createAlertRule(payload);
      return resp;
    },
    async update(id, payload) {
      const resp = await api_alertRule.updateAlertRule(id, payload);
      return resp;
    },
    async toggle(id, enabled) {
      await api_alertRule.toggleAlertRule(id, enabled);
      const updateLocal = (arr) => {
        const idx = (arr || []).findIndex((r) => (r == null ? void 0 : r.id) === id);
        if (idx >= 0)
          arr[idx].enabled = enabled;
      };
      updateLocal(this.rules);
      updateLocal(this.userRules);
      updateLocal(this.templates);
    },
    async remove(id) {
      await api_alertRule.deleteAlertRule(id);
      const filterOut = (arr) => (arr || []).filter((r) => (r == null ? void 0 : r.id) !== id);
      this.rules = filterOut(this.rules);
      this.userRules = filterOut(this.userRules);
      this.templates = filterOut(this.templates);
    }
  }
});
exports.useAlertRuleStore = useAlertRuleStore;
//# sourceMappingURL=../../.sourcemap/mp-weixin/stores/alertRule.js.map
