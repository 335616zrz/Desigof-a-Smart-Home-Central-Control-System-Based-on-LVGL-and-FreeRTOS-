"use strict";
const defaults = {
  baseURL: "",
  header: {},
  method: "GET",
  dataType: "json",
  responseType: "text",
  custom: {},
  timeout: 6e4,
  validateStatus: function validateStatus(status) {
    return status >= 200 && status < 300;
  }
};
exports.defaults = defaults;
//# sourceMappingURL=../../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/libs/luch-request/core/defaults.js.map
