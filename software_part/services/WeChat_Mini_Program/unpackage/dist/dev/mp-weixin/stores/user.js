"use strict";
const api_user = require("../api/user.js");
const stores_pinia = require("./pinia.js");
const useUserStore = stores_pinia.defineStore("user", {
  state: () => ({
    items: [],
    total: 0,
    page: 1,
    pageSize: 20,
    loading: false
  }),
  actions: {
    async fetchList(page = 1, pageSize = 20) {
      this.loading = true;
      try {
        const resp = await api_user.listUsers(page, pageSize);
        this.items = (resp == null ? void 0 : resp.items) || [];
        this.total = (resp == null ? void 0 : resp.total) || 0;
        this.page = page;
        this.pageSize = pageSize;
        return resp;
      } finally {
        this.loading = false;
      }
    },
    async create(payload) {
      const resp = await api_user.createUser(payload);
      return resp;
    },
    async remove(id) {
      await api_user.deleteUser(id);
      this.items = (this.items || []).filter((u) => (u == null ? void 0 : u.id) !== id);
      this.total = Math.max(0, (this.total || 0) - 1);
    }
  }
});
exports.useUserStore = useUserStore;
//# sourceMappingURL=../../.sourcemap/mp-weixin/stores/user.js.map
