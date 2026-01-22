"use strict";
const common_vendor = require("../common/vendor.js");
let activePinia = null;
function setActivePinia(pinia) {
  activePinia = pinia;
}
function getActivePinia() {
  return activePinia;
}
function createPinia() {
  const pinia = {
    state: common_vendor.reactive({}),
    _s: /* @__PURE__ */ new Map(),
    install(app) {
      setActivePinia(pinia);
      app.provide("pinia", pinia);
      app.config.globalProperties.$pinia = pinia;
    }
  };
  return pinia;
}
function defineStore(id, options) {
  return function useStore() {
    const pinia = getActivePinia();
    if (!pinia) {
      throw new Error("Pinia is not active. Did you forget to call app.use(createPinia())?");
    }
    if (pinia._s.has(id))
      return pinia._s.get(id);
    const rawState = typeof (options == null ? void 0 : options.state) === "function" ? options.state() : {};
    const state = common_vendor.reactive(rawState);
    const store = state;
    store.$id = id;
    const actions = (options == null ? void 0 : options.actions) || {};
    Object.keys(actions).forEach((key) => {
      const fn = actions[key];
      if (typeof fn === "function")
        store[key] = fn.bind(store);
    });
    const getters = (options == null ? void 0 : options.getters) || {};
    Object.keys(getters).forEach((key) => {
      const getter = getters[key];
      if (typeof getter !== "function")
        return;
      Object.defineProperty(store, key, {
        enumerable: true,
        get() {
          return common_vendor.computed(() => getter.call(store, store)).value;
        }
      });
    });
    pinia._s.set(id, store);
    return store;
  };
}
exports.createPinia = createPinia;
exports.defineStore = defineStore;
//# sourceMappingURL=../../.sourcemap/mp-weixin/stores/pinia.js.map
