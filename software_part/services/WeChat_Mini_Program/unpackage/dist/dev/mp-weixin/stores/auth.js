"use strict";
const common_vendor = require("../common/vendor.js");
const api_auth = require("../api/auth.js");
const common_constants = require("../common/constants.js");
const common_storage = require("../common/storage.js");
const stores_pinia = require("./pinia.js");
function normalizeAuthPayload(resp) {
  return (resp == null ? void 0 : resp.data) ?? resp ?? null;
}
const useAuthStore = stores_pinia.defineStore("auth", {
  state: () => ({
    user: null,
    token: "",
    refreshToken: "",
    isAuthenticated: false
  }),
  actions: {
    initFromStorage() {
      const token = common_storage.getString(common_constants.STORAGE_KEYS.TOKEN, "");
      const refreshToken = common_storage.getString(common_constants.STORAGE_KEYS.REFRESH_TOKEN, "");
      const user = common_storage.getJSON(common_constants.STORAGE_KEYS.USER, null);
      this.token = token;
      this.refreshToken = refreshToken;
      this.user = user;
      this.isAuthenticated = Boolean(token);
    },
    async login(username, password) {
      const resp = await api_auth.login(username, password);
      const payload = normalizeAuthPayload(resp);
      common_vendor.index.__f__("log", "at stores/auth.js:33", "[auth.js] login response payload:", JSON.stringify(payload));
      if (!payload)
        return null;
      this.token = payload.accessToken || "";
      this.refreshToken = payload.refreshToken || "";
      this.user = {
        username: payload.username,
        role: payload.role
      };
      this.isAuthenticated = Boolean(this.token);
      common_vendor.index.__f__("log", "at stores/auth.js:44", "[auth.js] saving token:", this.token ? `${this.token.substring(0, 20)}...` : "(empty)");
      common_storage.setString(common_constants.STORAGE_KEYS.TOKEN, this.token);
      common_storage.setString(common_constants.STORAGE_KEYS.REFRESH_TOKEN, this.refreshToken);
      common_storage.setJSON(common_constants.STORAGE_KEYS.USER, this.user);
      common_storage.setString(common_constants.STORAGE_KEYS.LAST_LOGIN_AT, String(Date.now()));
      return payload;
    },
    async register(username, password, email) {
      const resp = await api_auth.register(username, password, email);
      const payload = normalizeAuthPayload(resp);
      if (!payload)
        return null;
      this.token = payload.accessToken || "";
      this.refreshToken = payload.refreshToken || "";
      this.user = {
        username: payload.username,
        role: payload.role
      };
      this.isAuthenticated = Boolean(this.token);
      common_storage.setString(common_constants.STORAGE_KEYS.TOKEN, this.token);
      common_storage.setString(common_constants.STORAGE_KEYS.REFRESH_TOKEN, this.refreshToken);
      common_storage.setJSON(common_constants.STORAGE_KEYS.USER, this.user);
      common_storage.setString(common_constants.STORAGE_KEYS.LAST_LOGIN_AT, String(Date.now()));
      return payload;
    },
    async fetchProfile() {
      const resp = await api_auth.fetchProfile();
      const profile = normalizeAuthPayload(resp);
      if (!profile)
        return null;
      this.user = profile;
      common_storage.setJSON(common_constants.STORAGE_KEYS.USER, this.user);
      return profile;
    },
    logout() {
      this.user = null;
      this.token = "";
      this.refreshToken = "";
      this.isAuthenticated = false;
      common_storage.remove(common_constants.STORAGE_KEYS.TOKEN);
      common_storage.remove(common_constants.STORAGE_KEYS.REFRESH_TOKEN);
      common_storage.remove(common_constants.STORAGE_KEYS.USER);
      try {
        common_vendor.index.reLaunch({ url: common_constants.ROUTES.LOGIN });
      } catch (e) {
      }
    }
  }
});
exports.useAuthStore = useAuthStore;
//# sourceMappingURL=../../.sourcemap/mp-weixin/stores/auth.js.map
