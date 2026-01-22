"use strict";
function settle(resolve, reject, response) {
  const { validateStatus } = response.config;
  const status = response.statusCode;
  if (status && (!validateStatus || validateStatus(status))) {
    resolve(response);
  } else {
    reject(response);
  }
}
exports.settle = settle;
//# sourceMappingURL=../../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/libs/luch-request/core/settle.js.map
