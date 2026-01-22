"use strict";
const stores_pinia = require("./pinia.js");
let _pinia = null;
function setupStore(app) {
  _pinia = stores_pinia.createPinia();
  app.use(_pinia);
  return _pinia;
}
exports.setupStore = setupStore;
//# sourceMappingURL=../../.sourcemap/mp-weixin/stores/index.js.map
