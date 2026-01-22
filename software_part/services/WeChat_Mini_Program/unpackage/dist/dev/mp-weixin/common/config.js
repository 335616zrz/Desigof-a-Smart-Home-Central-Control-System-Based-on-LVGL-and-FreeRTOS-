"use strict";
const common_vendor = require("./vendor.js");
const ENV = {
  DEVELOP: "develop",
  TRIAL: "trial",
  RELEASE: "release"
};
const BASE_URL_MAP = {
  // Default to local backend (see backend `server.port` in `IoT_cloud_platform/backend`).
  // Update these URLs when deploying to a real HTTPS domain (required by WeChat in production).
  [ENV.DEVELOP]: "http://localhost:8080",
  [ENV.TRIAL]: "http://localhost:8080",
  [ENV.RELEASE]: "http://localhost:8080"
};
const REQUEST_TIMEOUT_MS = 15e3;
function getWxEnvVersion() {
  var _a, _b, _c;
  try {
    const info = (_b = (_a = common_vendor.wx$1) == null ? void 0 : _a.getAccountInfoSync) == null ? void 0 : _b.call(_a);
    return ((_c = info == null ? void 0 : info.miniProgram) == null ? void 0 : _c.envVersion) || ENV.RELEASE;
  } catch (e) {
    return ENV.RELEASE;
  }
  return ENV.RELEASE;
}
function getBaseUrl() {
  const env = getWxEnvVersion();
  return BASE_URL_MAP[env] || BASE_URL_MAP[ENV.RELEASE];
}
exports.REQUEST_TIMEOUT_MS = REQUEST_TIMEOUT_MS;
exports.getBaseUrl = getBaseUrl;
//# sourceMappingURL=../../.sourcemap/mp-weixin/common/config.js.map
